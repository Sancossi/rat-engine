#include <rat/event_edit.hpp>
#include <rat/event_runtime.hpp>
#include <rat/hot_apply.hpp>
#include <rat/map_document.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("tile event world position is cell center", "[unit][event_edit]") {
  const auto center = rat::tile_center_world({2, -1}, 1.0f);
  REQUIRE(center.x == Approx(2.5f));
  REQUIRE(center.z == Approx(-0.5f));
}

TEST_CASE("make_stub_event creates Action page on tile", "[unit][event_edit]") {
  const auto event = rat::make_stub_event("npc_new", 3, -1);
  REQUIRE(event.id == "npc_new");
  REQUIRE(event.tile.has_value());
  REQUIRE(event.tile->x == 3);
  REQUIRE(event.tile->z == -1);
  REQUIRE(event.pages.size() == 1);
  REQUIRE(event.pages[0].trigger == rat::TriggerKind::Action);
  REQUIRE(event.pages[0].commands[0].op == rat::CommandOp::ShowText);
}

TEST_CASE("translate_event_on_grid moves tile and volume", "[unit][event_edit]") {
  rat::EventDef event;
  event.tile = rat::TileCoord{1, 2};
  event.volume = rat::Aabb2{0.0f, 0.0f, 1.0f, 1.0f};
  rat::translate_event_on_grid(event, 2, -1, 1.0f);
  REQUIRE(event.tile->x == 3);
  REQUIRE(event.tile->z == 1);
  REQUIRE(event.volume->min_x == Approx(2.0f));
  REQUIRE(event.volume->max_x == Approx(3.0f));
  REQUIRE(event.volume->min_z == Approx(-1.0f));
  REQUIRE(event.volume->max_z == Approx(0.0f));
}

TEST_CASE("event_marker_index skips events without placement", "[unit][event_edit]") {
  rat::MapData map;
  map.events.push_back(rat::make_stub_event("a", 0, 0));
  rat::EventDef orphan;
  orphan.id = "ghost";
  orphan.pages.push_back({});
  map.events.push_back(orphan);
  map.events.push_back(rat::make_stub_event("b", 1, 1));

  REQUIRE(rat::event_marker_index(map, 0) == 0);
  REQUIRE(rat::event_marker_index(map, 1) == -1);
  REQUIRE(rat::event_marker_index(map, 2) == 1);
}

TEST_CASE("compile and reload events updates markers path via map", "[unit][event_edit]") {
  rat::EventRuntime runtime;
  rat::MapData map;
  map.id = "t";
  map.width = 4;
  map.height = 4;
  map.events.push_back(rat::make_stub_event("a", 0, 0));
  REQUIRE(runtime.load(map).ok);

  auto events = runtime.map().events;
  rat::translate_event_on_grid(events[0], 4, 0, 1.0f);
  map.events = events;
  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);
  runtime.load(compiled.runtime);
  REQUIRE(runtime.map().events[0].tile->x == 4);

  const auto markers = rat::event_markers_from_map(runtime.map());
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].x == Approx(4.5f));
}
