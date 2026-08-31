#include <rat/camera.hpp>
#include <rat/event_edit.hpp>
#include <rat/viewport_edit.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace {

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
  const float expected_x = params.focus.x + ndc_x * camera.half_width_world;
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

TEST_CASE("pick returns nearest and prefers blocker on tie", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  map.events.push_back(rat::make_stub_event("e0", 0, 0));

  const auto picked = rat::pick_map_object_xz(map, rat::Vec3{0.5f, 0.0f, 0.5f});
  REQUIRE(picked.has_value());
  REQUIRE(picked->kind == rat::ViewportPickKind::Blocker);
  REQUIRE(picked->index == 0);
}

TEST_CASE("place tool resolves empty click into tile placement", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::Vec3 world{2.9f, 0.0f, -0.1f};

  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceBlocker, world);
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceBlocker);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == -1);
}

TEST_CASE("place tool click on object selects instead of placing", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  map.blockers.push_back(make_blocker(2.0f, 1.0f, 3.0f, 2.0f));

  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceEvent, rat::Vec3{2.2f, 0.0f, 1.6f});
  REQUIRE(action.kind == rat::ViewportClickActionKind::SelectBlocker);
  REQUIRE(action.index == 0);
}
