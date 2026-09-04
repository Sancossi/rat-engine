#include <rat/camera.hpp>
#include <rat/event_edit.hpp>
#include <rat/height_edit.hpp>
#include <rat/hot_apply.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>
#include <rat/terrain_geometry.hpp>
#include <rat/viewport_edit.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using Catch::Approx;

namespace {

rat::Vec3 vec_sub(rat::Vec3 a, rat::Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

float vec_dot(rat::Vec3 a, rat::Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

rat::Vec3 vec_cross(rat::Vec3 a, rat::Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

rat::Vec3 vec_normalize(rat::Vec3 v) {
  const float len = std::sqrt(vec_dot(v, v));
  return {v.x / len, v.y / len, v.z / len};
}

// Byte-layout twin of bx::mtxLookAt(..., Left). rat_core must not link bx.
rat::Mat4 look_at_bx_left(rat::Vec3 eye, rat::Vec3 at, rat::Vec3 up) {
  const rat::Vec3 view = vec_normalize(vec_sub(at, eye));
  const rat::Vec3 uxv = vec_cross(up, view);
  const rat::Vec3 right = (vec_dot(uxv, uxv) == 0.0f) ? rat::Vec3{-1.0f, 0.0f, 0.0f} : vec_normalize(uxv);
  const rat::Vec3 up_axis = vec_cross(view, right);

  rat::Mat4 out{};
  out.m[0] = right.x;
  out.m[1] = up_axis.x;
  out.m[2] = view.x;
  out.m[3] = 0.0f;
  out.m[4] = right.y;
  out.m[5] = up_axis.y;
  out.m[6] = view.y;
  out.m[7] = 0.0f;
  out.m[8] = right.z;
  out.m[9] = up_axis.z;
  out.m[10] = view.z;
  out.m[11] = 0.0f;
  out.m[12] = -vec_dot(right, eye);
  out.m[13] = -vec_dot(up_axis, eye);
  out.m[14] = -vec_dot(view, eye);
  out.m[15] = 1.0f;
  return out;
}

rat::MapData make_test_map() {
  rat::MapData map;
  map.id = "viewport";
  map.width = 8;
  map.height = 8;
  map.tile_size = 1.0f;
  return map;
}

rat::MapData make_elevated_map() {
  rat::MapData map = make_test_map();
  map.schema_version = 2;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 8;
  map.height_grid.height = 8;
  map.height_grid.ground_y.assign(64, 0.0f);
  return map;
}

rat::MapData make_east_ramp_map_with_event() {
  rat::MapData map = make_elevated_map();
  rat::RampDef ramp;
  ramp.tile = {2, 2};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 0.0f;
  ramp.high_y = 1.0f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);
  map.events.push_back(rat::make_stub_event("on_ramp", 2, 2));
  return map;
}

rat::BlockerDef make_blocker(float min_x, float min_z, float max_x, float max_z) {
  rat::BlockerDef blocker;
  blocker.bounds = {min_x, min_z, max_x, max_z};
  return blocker;
}

}  // namespace

TEST_CASE("unproject center pixel hits camera focus xz", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {3.0f, 0.0f, -2.0f};
  params.mode = rat::CameraMode::TopDown;
  const rat::OrthoCamera camera = rat::build_ortho_top_down(640, 480, params);

  const auto hit = rat::unproject_to_ground_plane(camera, 320.0f, 240.0f, 640, 480);
  REQUIRE(hit.has_value());
  REQUIRE(hit->x == Approx(params.focus.x).margin(0.01f));
  REQUIRE(hit->y == Approx(0.0f).margin(0.001f));
  REQUIRE(hit->z == Approx(params.focus.z).margin(0.01f));
}

TEST_CASE("project unprojected center pixel round-trips to screen center", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {3.0f, 0.0f, -2.0f};
  params.mode = rat::CameraMode::TopDown;
  const rat::OrthoCamera camera = rat::build_ortho_top_down(640, 480, params);

  const auto hit = rat::unproject_to_ground_plane(camera, 320.0f, 240.0f, 640, 480);
  REQUIRE(hit.has_value());

  const auto pixel = rat::project_world_to_pixels(camera, *hit, 640, 480);
  REQUIRE(pixel.has_value());
  REQUIRE(pixel->x == Approx(320.0f).margin(2.0f));
  REQUIRE(pixel->y == Approx(240.0f).margin(2.0f));
}

TEST_CASE("unproject off-center pixel follows top-down extents", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {3.0f, 0.0f, -2.0f};
  params.mode = rat::CameraMode::TopDown;
  const rat::OrthoCamera camera = rat::build_ortho_top_down(640, 480, params);

  const float pixel_x = 400.0f;
  const float pixel_y = 180.0f;
  const auto hit = rat::unproject_to_ground_plane(camera, pixel_x, pixel_y, 640, 480);
  REQUIRE(hit.has_value());

  const float ndc_x = 2.0f * pixel_x / 640.0f - 1.0f;
  const float ndc_y = 1.0f - 2.0f * pixel_y / 480.0f;
  // TopDown look_at Left: right = (-1,0,0), camera-up = (0,0,-1). Screen-up = world -Z.
  const float expected_x = params.focus.x - ndc_x * camera.half_width_world;
  const float expected_z = params.focus.z - ndc_y * camera.half_height_world;
  REQUIRE(hit->x == Approx(expected_x).margin(0.02f));
  REQUIRE(hit->y == Approx(0.0f).margin(0.001f));
  REQUIRE(hit->z == Approx(expected_z).margin(0.02f));
}

TEST_CASE("unproject tilt45 center pixel hits focus xz", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {5.0f, 0.0f, 7.0f};
  const rat::OrthoCamera camera = rat::build_ortho_tilt45(800, 600, params);

  const auto hit = rat::unproject_to_ground_plane(camera, 400.0f, 300.0f, 800, 600);
  REQUIRE(hit.has_value());
  REQUIRE(hit->x == Approx(params.focus.x).margin(0.05f));
  REQUIRE(hit->z == Approx(params.focus.z).margin(0.05f));
}

TEST_CASE("unproject three-quarter center pixel hits focus xz", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {-4.0f, 0.0f, 6.0f};
  const rat::OrthoCamera camera = rat::build_ortho_three_quarter(800, 600, params);

  const auto hit = rat::unproject_to_ground_plane(camera, 400.0f, 300.0f, 800, 600);
  REQUIRE(hit.has_value());
  REQUIRE(hit->x == Approx(params.focus.x).margin(0.10f));
  REQUIRE(hit->z == Approx(params.focus.z).margin(0.10f));
}

TEST_CASE("unproject bx look-at left view hits center and off-center", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {3.0f, 0.0f, -2.0f};
  params.mode = rat::CameraMode::TopDown;
  rat::OrthoCamera camera = rat::build_ortho_top_down(640, 480, params);
  camera.view = look_at_bx_left(camera.eye, params.focus, {0.0f, 0.0f, -1.0f});

  const auto center = rat::unproject_to_ground_plane(camera, 320.0f, 240.0f, 640, 480);
  REQUIRE(center.has_value());
  REQUIRE(center->x == Approx(params.focus.x).margin(0.01f));
  REQUIRE(center->y == Approx(0.0f).margin(0.001f));
  REQUIRE(center->z == Approx(params.focus.z).margin(0.01f));

  const float pixel_x = 400.0f;
  const float pixel_y = 180.0f;
  const auto off = rat::unproject_to_ground_plane(camera, pixel_x, pixel_y, 640, 480);
  REQUIRE(off.has_value());

  const float ndc_x = 2.0f * pixel_x / 640.0f - 1.0f;
  const float ndc_y = 1.0f - 2.0f * pixel_y / 480.0f;
  // bx Left TopDown: right = (-1,0,0), camera-up = (0,0,-1).
  const float expected_x = params.focus.x - ndc_x * camera.half_width_world;
  const float expected_z = params.focus.z - ndc_y * camera.half_height_world;
  REQUIRE(off->x == Approx(expected_x).margin(0.02f));
  REQUIRE(off->y == Approx(0.0f).margin(0.001f));
  REQUIRE(off->z == Approx(expected_z).margin(0.02f));
}

TEST_CASE("objects pick returns blocker and ignores overlapping event", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  map.events.push_back(rat::make_stub_event("e0", 0, 0));

  const auto picked =
      rat::pick_map_object_xz(map, rat::Vec3{0.5f, 0.0f, 0.5f}, rat::EditSubmode::Objects);
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Blocker);
  REQUIRE(picked->index == 0);
}

TEST_CASE("terrain pick returns nullopt over blocker and event", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  map.events.push_back(rat::make_stub_event("e0", 0, 0));

  const auto picked =
      rat::pick_map_object_xz(map, rat::Vec3{0.5f, 0.0f, 0.5f}, rat::EditSubmode::Terrain);
  REQUIRE_FALSE(picked.has_value());
}

TEST_CASE("events pick returns event and ignores overlapping blocker", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  map.events.push_back(rat::make_stub_event("e0", 0, 0));

  const auto picked =
      rat::pick_map_object_xz(map, rat::Vec3{0.5f, 0.0f, 0.5f}, rat::EditSubmode::Events);
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Event);
  REQUIRE(picked->index == 0);
}

TEST_CASE("place tool resolves empty click into tile placement", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 world{2.9f, 0.0f, -0.1f};

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceBlocker, world, rat::EditSubmode::Objects);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceBlocker);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == -1);
}

TEST_CASE("objects place blocker on existing blocker still selects", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(2.0f, 1.0f, 3.0f, 2.0f));

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceBlocker, rat::Vec3{2.2f, 0.0f, 1.6f}, rat::EditSubmode::Objects);
  REQUIRE(action.kind == rat::ViewportClickActionKind::SelectBlocker);
  REQUIRE(action.index == 0);
}

TEST_CASE("place cube on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 world{2.9f, 0.0f, -0.1f};

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceCube, world, rat::EditSubmode::Terrain);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceCube);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == -1);
}

TEST_CASE("place cube on ramp tile does not raise ground", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.schema_version = 2;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 8;
  map.height_grid.height = 8;
  map.height_grid.ground_y.assign(64, 0.0f);

  rat::RampDef ramp;
  ramp.tile = {1, 2};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 0.0f;
  ramp.high_y = 1.0f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);

  const rat::Vec3 world{1.2f, 0.0f, 2.3f};
  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceCube, world, rat::EditSubmode::Terrain);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceCube);
  REQUIRE(action.tile.x == 1);
  REQUIRE(action.tile.z == 2);

  const auto before_grid = map.height_grid.ground_y;
  const auto before_ramps = map.ramps;
  const auto placed = rat::place_map_tile_cube(map, action.tile.x, action.tile.z);
  REQUIRE_FALSE(placed.ok);
  REQUIRE(map.height_grid.ground_y == before_grid);
  REQUIRE(map.ramps.size() == before_ramps.size());
}

TEST_CASE("place fence on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 world{4.1f, 0.0f, 3.7f};

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceFence, world, rat::EditSubmode::Terrain);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceFence);
  REQUIRE(action.tile.x == 4);
  REQUIRE(action.tile.z == 3);
}

TEST_CASE("terrain place cube and fence on blocker aabb still place", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(2.0f, 1.0f, 3.0f, 2.0f));
  const rat::Vec3 hit{2.2f, 0.0f, 1.6f};

  const rat::ViewportClickAction cube =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceCube, hit, rat::EditSubmode::Terrain);
  REQUIRE(cube.kind == rat::ViewportClickActionKind::PlaceCube);
  REQUIRE(cube.tile.x == 2);
  REQUIRE(cube.tile.z == 1);

  const rat::ViewportClickAction fence = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceFence, hit, rat::EditSubmode::Terrain);
  REQUIRE(fence.kind == rat::ViewportClickActionKind::PlaceFence);
  REQUIRE(fence.tile.x == 2);
  REQUIRE(fence.tile.z == 1);

  const rat::ViewportClickAction slab =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceSlab, hit, rat::EditSubmode::Terrain);
  REQUIRE(slab.kind == rat::ViewportClickActionKind::PlaceSlab);
  REQUIRE(slab.tile.x == 2);
  REQUIRE(slab.tile.z == 1);

  const rat::ViewportClickAction ramp = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceRamp, hit, rat::EditSubmode::Terrain);
  REQUIRE(ramp.kind == rat::ViewportClickActionKind::PlaceRamp);
  REQUIRE(ramp.tile.x == 2);
  REQUIRE(ramp.tile.z == 1);
}

TEST_CASE("events click overlapping blocker selects event", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  map.events.push_back(rat::make_stub_event("e0", 0, 0));

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::Select, rat::Vec3{0.5f, 0.0f, 0.5f}, rat::EditSubmode::Events);
  REQUIRE(action.kind == rat::ViewportClickActionKind::SelectEvent);
  REQUIRE(action.index == 0);
}

TEST_CASE("place slab on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceSlab, rat::Vec3{2.9f, 0.0f, -0.1f},
                                 rat::EditSubmode::Terrain);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceSlab);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == -1);
}

TEST_CASE("place bridge on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceBridge, rat::Vec3{2.9f, 0.0f, -0.1f},
                                 rat::EditSubmode::Terrain);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceBridge);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == -1);
}

TEST_CASE("place ladder on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceLadder, rat::Vec3{4.1f, 0.0f, 3.7f},
                                 rat::EditSubmode::Objects);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceLadder);
  REQUIRE(action.tile.x == 4);
  REQUIRE(action.tile.z == 3);
}

TEST_CASE("place ramp on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 world{4.1f, 0.0f, 3.7f};

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceRamp, world, rat::EditSubmode::Terrain);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceRamp);
  REQUIRE(action.tile.x == 4);
  REQUIRE(action.tile.z == 3);
}

TEST_CASE("nearest tile edge is the closest of N E S W", "[unit][edit][viewport_edit]") {
  constexpr float tile = 1.0f;
  // Tile (2,3) occupies [2,3] x [3,4]. North = min Z, South = max Z, West = min X, East = max X.
  REQUIRE(rat::nearest_tile_edge({2.5f, 0.0f, 3.05f}, tile) == rat::RampDirection::North);
  REQUIRE(rat::nearest_tile_edge({2.95f, 0.0f, 3.5f}, tile) == rat::RampDirection::East);
  REQUIRE(rat::nearest_tile_edge({2.5f, 0.0f, 3.95f}, tile) == rat::RampDirection::South);
  REQUIRE(rat::nearest_tile_edge({2.05f, 0.0f, 3.5f}, tile) == rat::RampDirection::West);
}

TEST_CASE("place fence near opposite faces of the same cell pick those facings",
          "[unit][edit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 east_hit{2.92f, 0.0f, 3.50f};
  const rat::Vec3 west_hit{2.08f, 0.0f, 3.50f};

  const rat::ViewportClickAction east =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceFence, east_hit,
                                 rat::EditSubmode::Terrain);
  REQUIRE(east.kind == rat::ViewportClickActionKind::PlaceFence);
  REQUIRE(east.tile.x == 2);
  REQUIRE(east.tile.z == 3);
  REQUIRE(east.edge == rat::RampDirection::East);

  const rat::ViewportClickAction west =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceFence, west_hit,
                                 rat::EditSubmode::Terrain);
  REQUIRE(west.kind == rat::ViewportClickActionKind::PlaceFence);
  REQUIRE(west.tile.x == 2);
  REQUIRE(west.tile.z == 3);
  REQUIRE(west.edge == rat::RampDirection::West);
}

TEST_CASE("place ladder click uses the nearest tile edge", "[unit][edit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceLadder, rat::Vec3{1.50f, 0.0f, 4.97f},
                                 rat::EditSubmode::Objects);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceLadder);
  REQUIRE(action.tile.x == 1);
  REQUIRE(action.tile.z == 4);
  REQUIRE(action.edge == rat::RampDirection::South);
}

TEST_CASE("place ramp near opposite faces of the same cell pick those facings",
          "[unit][edit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 east_hit{2.92f, 0.0f, 3.50f};
  const rat::Vec3 west_hit{2.08f, 0.0f, 3.50f};

  const rat::ViewportClickAction east =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceRamp, east_hit,
                                 rat::EditSubmode::Terrain);
  REQUIRE(east.kind == rat::ViewportClickActionKind::PlaceRamp);
  REQUIRE(east.tile.x == 2);
  REQUIRE(east.tile.z == 3);
  REQUIRE(east.edge == rat::RampDirection::East);

  const rat::ViewportClickAction west =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceRamp, west_hit,
                                 rat::EditSubmode::Terrain);
  REQUIRE(west.kind == rat::ViewportClickActionKind::PlaceRamp);
  REQUIRE(west.tile.x == 2);
  REQUIRE(west.tile.z == 3);
  REQUIRE(west.edge == rat::RampDirection::West);
}

TEST_CASE("place ramp west and south of a raised cell both climb in play",
          "[unit][viewport_edit][player][surface]") {
  rat::MapData map = make_elevated_map();
  REQUIRE(rat::place_map_tile_cube(map, 1, 1).ok);
  REQUIRE(rat::get_tile_ground_y(map.height_grid, 1, 1).value == Approx(1.0f));

  // West neighbor (0,1): click east edge so high side faces the cube.
  const rat::ViewportClickAction west_approach = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceRamp, rat::Vec3{0.92f, 0.0f, 1.50f}, rat::EditSubmode::Terrain);
  REQUIRE(west_approach.kind == rat::ViewportClickActionKind::PlaceRamp);
  REQUIRE(west_approach.tile.x == 0);
  REQUIRE(west_approach.tile.z == 1);
  REQUIRE(west_approach.edge == rat::RampDirection::East);

  // South neighbor (1,2): click north edge so high side faces the cube.
  const rat::ViewportClickAction south_approach = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceRamp, rat::Vec3{1.50f, 0.0f, 2.08f}, rat::EditSubmode::Terrain);
  REQUIRE(south_approach.kind == rat::ViewportClickActionKind::PlaceRamp);
  REQUIRE(south_approach.tile.x == 1);
  REQUIRE(south_approach.tile.z == 2);
  REQUIRE(south_approach.edge == rat::RampDirection::North);

  rat::RampDef west_ramp;
  west_ramp.tile = west_approach.tile;
  west_ramp.direction = west_approach.edge;
  west_ramp.low_y = 0.0f;
  west_ramp.high_y = 1.0f;
  REQUIRE(rat::upsert_map_ramp(map, west_ramp).ok);

  rat::RampDef south_ramp;
  south_ramp.tile = south_approach.tile;
  south_ramp.direction = south_approach.edge;
  south_ramp.low_y = 0.0f;
  south_ramp.high_y = 1.0f;
  REQUIRE(rat::upsert_map_ramp(map, south_ramp).ok);
  REQUIRE(map.ramps.size() == 2);

  const rat::SurfaceQuery query(map);
  const rat::SurfaceSample west_mid = query.sample(0.5f, 1.5f);
  REQUIRE(west_mid.on_ramp);
  REQUIRE(west_mid.y == Approx(0.5f).margin(0.05f));
  const rat::SurfaceSample south_mid = query.sample(1.5f, 2.5f);
  REQUIRE(south_mid.on_ramp);
  REQUIRE(south_mid.y == Approx(0.5f).margin(0.05f));
  REQUIRE(query.sample(1.5f, 1.5f).y == Approx(1.0f).margin(0.05f));

  auto climb = [&](rat::PlayerBody player, rat::MoveInput input) {
    float prev_y = player.y;
    for (int i = 0; i < 24; ++i) {
      player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f);
      REQUIRE(player.y >= prev_y - 1e-4f);
      prev_y = player.y;
    }
    return player;
  };

  rat::PlayerBody from_west;
  from_west.x = 0.05f;
  from_west.y = 0.0f;
  from_west.z = 1.5f;
  from_west.half_extent = 0.8f;
  from_west.speed = 3.0f;
  from_west = climb(from_west, rat::MoveInput{1.0f, 0.0f});
  REQUIRE(from_west.x > 1.05f);
  REQUIRE(from_west.y == Approx(1.0f).margin(0.05f));

  rat::PlayerBody from_south;
  from_south.x = 1.5f;
  from_south.y = 0.0f;
  from_south.z = 2.95f;
  from_south.half_extent = 0.8f;
  from_south.speed = 3.0f;
  from_south = climb(from_south, rat::MoveInput{0.0f, -1.0f});
  REQUIRE(from_south.z < 1.95f);
  REQUIRE(from_south.y == Approx(1.0f).margin(0.05f));
}

TEST_CASE("drag along a north wall picks north on each adjacent tile",
          "[unit][edit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction first =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceFence, rat::Vec3{0.50f, 0.0f, 0.05f},
                                 rat::EditSubmode::Terrain);
  const rat::ViewportClickAction second = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceFence, rat::Vec3{1.50f, 0.0f, 0.05f}, rat::EditSubmode::Terrain);
  REQUIRE(first.kind == rat::ViewportClickActionKind::PlaceFence);
  REQUIRE(first.tile.x == 0);
  REQUIRE(first.tile.z == 0);
  REQUIRE(first.edge == rat::RampDirection::North);
  REQUIRE(second.kind == rat::ViewportClickActionKind::PlaceFence);
  REQUIRE(second.tile.x == 1);
  REQUIRE(second.tile.z == 0);
  REQUIRE(second.edge == rat::RampDirection::North);
}

TEST_CASE("events place event on blocker aabb still places", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(2.0f, 1.0f, 3.0f, 2.0f));

  const rat::ViewportClickAction action = rat::resolve_viewport_click(
      map, rat::ViewportTool::PlaceEvent, rat::Vec3{2.2f, 0.0f, 1.6f}, rat::EditSubmode::Events);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceEvent);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == 1);
}

TEST_CASE("viewport tool allowed matches edit submode", "[unit][viewport_edit]") {
  using rat::EditSubmode;
  using rat::ViewportTool;

  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::Select));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceCube));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceFence));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceSlab));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceRamp));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceBridge));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceBlocker));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceEvent));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceLadder));

  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::Select));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceBlocker));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceLadder));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceCube));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceFence));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceSlab));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceRamp));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceBridge));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceEvent));

  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::Select));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceEvent));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceBlocker));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceCube));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceFence));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceSlab));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceLadder));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceRamp));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceBridge));
}

TEST_CASE("unproject y=0 misses east ramp tile under tilt45", "[unit][viewport_edit]") {
  const rat::MapData map = make_east_ramp_map_with_event();
  const auto markers = rat::event_markers_from_map(map);
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].y == Approx(0.5f).margin(0.01f));

  rat::OrthoCameraParams params;
  params.focus = {2.5f, 0.0f, 2.5f};
  const rat::OrthoCamera camera = rat::build_ortho_tilt45(800, 600, params);
  const auto pixel = rat::project_world_to_pixels(camera, markers[0], 800, 600);
  REQUIRE(pixel.has_value());

  const auto hit = rat::unproject_to_ground_plane(camera, pixel->x, pixel->y, 800, 600, 0.0f);
  REQUIRE(hit.has_value());
  const rat::TileCoord tile = rat::world_to_tile_xz(*hit, map.tile_size);
  REQUIRE_FALSE((tile.x == 2 && tile.z == 2));

  const auto picked = rat::pick_map_object_xz(map, *hit, rat::EditSubmode::Events);
  REQUIRE_FALSE(picked.has_value());
}

TEST_CASE("unproject to terrain selects event on ramp under tilt45", "[unit][viewport_edit]") {
  const rat::MapData map = make_east_ramp_map_with_event();
  const auto markers = rat::event_markers_from_map(map);
  REQUIRE(markers.size() == 1);

  rat::OrthoCameraParams params;
  params.focus = {2.5f, 0.0f, 2.5f};
  const rat::OrthoCamera camera = rat::build_ortho_tilt45(800, 600, params);
  const auto pixel = rat::project_world_to_pixels(camera, markers[0], 800, 600);
  REQUIRE(pixel.has_value());

  const rat::TerrainGeometry geometry =
      rat::build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  const auto hit =
      rat::unproject_to_terrain(camera, pixel->x, pixel->y, 800, 600, geometry);
  REQUIRE(hit.has_value());
  const rat::TileCoord tile = rat::world_to_tile_xz(*hit, map.tile_size);
  REQUIRE(tile.x == 2);
  REQUIRE(tile.z == 2);

  const auto picked = rat::pick_map_object_xz(map, *hit, rat::EditSubmode::Events);
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Event);
  REQUIRE(picked->index == 0);
}

TEST_CASE("unproject to terrain hits ramp high side under three-quarter", "[unit][viewport_edit]") {
  const rat::MapData map = make_east_ramp_map_with_event();
  const rat::TerrainGeometry geometry =
      rat::build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  const rat::Vec3 surface{2.95f, rat::sample_terrain_height(geometry, 2.95f, 2.05f), 2.05f};

  rat::OrthoCameraParams params;
  params.focus = {2.5f, 0.0f, 2.5f};
  const rat::OrthoCamera camera = rat::build_ortho_three_quarter(800, 600, params);
  const auto pixel = rat::project_world_to_pixels(camera, surface, 800, 600);
  REQUIRE(pixel.has_value());

  const auto hit =
      rat::unproject_to_terrain(camera, pixel->x, pixel->y, 800, 600, geometry);
  REQUIRE(hit.has_value());
  const rat::TileCoord tile = rat::world_to_tile_xz(*hit, map.tile_size);
  REQUIRE(tile.x == 2);
  REQUIRE(tile.z == 2);

  const auto picked = rat::pick_map_object_xz(map, *hit, rat::EditSubmode::Events);
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Event);
  REQUIRE(picked->index == 0);
}

TEST_CASE("unproject to terrain selects event on raised cube under tilt45", "[unit][viewport_edit]") {
  rat::MapData map = make_elevated_map();
  REQUIRE(rat::place_map_tile_cube(map, 3, 3).ok);
  map.events.push_back(rat::make_stub_event("on_cube", 3, 3));
  const auto markers = rat::event_markers_from_map(map);
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].y == Approx(1.0f).margin(0.01f));

  rat::OrthoCameraParams params;
  params.focus = {3.5f, 0.0f, 3.5f};
  const rat::OrthoCamera camera = rat::build_ortho_tilt45(800, 600, params);
  const auto pixel = rat::project_world_to_pixels(camera, markers[0], 800, 600);
  REQUIRE(pixel.has_value());

  const auto y0 = rat::unproject_to_ground_plane(camera, pixel->x, pixel->y, 800, 600, 0.0f);
  REQUIRE(y0.has_value());
  const rat::TileCoord y0_tile = rat::world_to_tile_xz(*y0, map.tile_size);
  REQUIRE_FALSE((y0_tile.x == 3 && y0_tile.z == 3));

  const rat::TerrainGeometry geometry =
      rat::build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  const auto hit =
      rat::unproject_to_terrain(camera, pixel->x, pixel->y, 800, 600, geometry);
  REQUIRE(hit.has_value());
  const rat::TileCoord tile = rat::world_to_tile_xz(*hit, map.tile_size);
  REQUIRE(tile.x == 3);
  REQUIRE(tile.z == 3);

  const auto picked = rat::pick_map_object_xz(map, *hit, rat::EditSubmode::Events);
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Event);
  REQUIRE(picked->index == 0);
}

TEST_CASE("unproject to terrain still picks ground event under top-down", "[unit][viewport_edit]") {
  rat::MapData map = make_elevated_map();
  map.events.push_back(rat::make_stub_event("ground", 1, 1));
  const auto markers = rat::event_markers_from_map(map);
  REQUIRE(markers.size() == 1);

  rat::OrthoCameraParams params;
  params.focus = {1.5f, 0.0f, 1.5f};
  params.mode = rat::CameraMode::TopDown;
  const rat::OrthoCamera camera = rat::build_ortho_top_down(640, 480, params);
  const auto pixel = rat::project_world_to_pixels(camera, markers[0], 640, 480);
  REQUIRE(pixel.has_value());

  const rat::TerrainGeometry geometry =
      rat::build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  const auto hit =
      rat::unproject_to_terrain(camera, pixel->x, pixel->y, 640, 480, geometry);
  REQUIRE(hit.has_value());
  REQUIRE(hit->x == Approx(1.5f).margin(0.05f));
  REQUIRE(hit->z == Approx(1.5f).margin(0.05f));

  const auto picked = rat::pick_map_object_xz(map, *hit, rat::EditSubmode::Events);
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Event);
  REQUIRE(picked->index == 0);
}

TEST_CASE("unproject to terrain falls back to y=0 when geometry is empty", "[unit][viewport_edit]") {
  rat::OrthoCameraParams params;
  params.focus = {3.0f, 0.0f, -2.0f};
  params.mode = rat::CameraMode::TopDown;
  const rat::OrthoCamera camera = rat::build_ortho_top_down(640, 480, params);

  const auto hit = rat::unproject_to_terrain(camera, 320.0f, 240.0f, 640, 480, {});
  REQUIRE(hit.has_value());
  REQUIRE(hit->x == Approx(params.focus.x).margin(0.01f));
  REQUIRE(hit->y == Approx(0.0f).margin(0.001f));
  REQUIRE(hit->z == Approx(params.focus.z).margin(0.01f));
}

TEST_CASE("unproject y=0 misses east ramp tile under three-quarter", "[unit][viewport_edit]") {
  const rat::MapData map = make_east_ramp_map_with_event();
  const rat::TerrainGeometry geometry =
      rat::build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  // High NE of the east ramp: enough Y that a 3/4 ray's y=0 XZ leaves the tile.
  const rat::Vec3 surface{2.95f, rat::sample_terrain_height(geometry, 2.95f, 2.05f), 2.05f};
  REQUIRE(surface.y == Approx(0.95f).margin(0.01f));
  REQUIRE(rat::world_to_tile_xz(surface, map.tile_size).x == 2);
  REQUIRE(rat::world_to_tile_xz(surface, map.tile_size).z == 2);

  rat::OrthoCameraParams params;
  params.focus = {2.5f, 0.0f, 2.5f};
  const rat::OrthoCamera camera = rat::build_ortho_three_quarter(800, 600, params);
  const auto pixel = rat::project_world_to_pixels(camera, surface, 800, 600);
  REQUIRE(pixel.has_value());

  const auto hit = rat::unproject_to_ground_plane(camera, pixel->x, pixel->y, 800, 600, 0.0f);
  REQUIRE(hit.has_value());
  const rat::TileCoord tile = rat::world_to_tile_xz(*hit, map.tile_size);
  REQUIRE_FALSE((tile.x == 2 && tile.z == 2));
}
