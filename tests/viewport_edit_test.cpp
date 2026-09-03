#include <rat/camera.hpp>
#include <rat/event_edit.hpp>
#include <rat/height_edit.hpp>
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

TEST_CASE("place ladder on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceLadder, rat::Vec3{4.1f, 0.0f, 3.7f},
                                 rat::EditSubmode::Objects);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceLadder);
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
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceBlocker));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceEvent));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Terrain, ViewportTool::PlaceLadder));

  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::Select));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceBlocker));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceLadder));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceCube));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceFence));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceSlab));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Objects, ViewportTool::PlaceEvent));

  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::Select));
  REQUIRE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceEvent));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceBlocker));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceCube));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceFence));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceSlab));
  REQUIRE_FALSE(rat::viewport_tool_allowed(EditSubmode::Events, ViewportTool::PlaceLadder));
}
