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
