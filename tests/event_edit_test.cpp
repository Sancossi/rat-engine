#include <rat/edit_history.hpp>
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

TEST_CASE("allocate_unique_event_id returns base when unused", "[unit][event_edit]") {
  rat::MapData map;
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  REQUIRE(rat::allocate_unique_event_id(map, "other") == "other");
}

TEST_CASE("allocate_unique_event_id suffixes _copy then _copy2", "[unit][event_edit]") {
  rat::MapData map;
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  REQUIRE(rat::allocate_unique_event_id(map, "npc") == "npc_copy");
  map.events.push_back(rat::make_stub_event("npc_copy", 1, 0));
  REQUIRE(rat::allocate_unique_event_id(map, "npc") == "npc_copy2");
}

TEST_CASE("make_duplicate_event clones pages graph y and offsets tile x by 1",
          "[unit][event_edit]") {
  rat::MapData map;
  rat::EventDef source = rat::make_stub_event("npc", 2, 5);
  source.y = 1.5f;
  source.volume = rat::Aabb2{2.0f, 5.0f, 3.0f, 6.0f};
  source.pages[0].graph = rat::EventGraph{};
  source.pages[0].graph->nodes.push_back({"n0", "show_text", "hi"});
  rat::Condition enable;
  enable.type = rat::ConditionType::Switch;
  enable.id = 3;
  source.pages[0].conditions.push_back(enable);
  map.events.push_back(source);

  const rat::EventDef dup = rat::make_duplicate_event(map, 0);
  REQUIRE(dup.id == "npc_copy");
  REQUIRE(dup.tile.has_value());
  REQUIRE(dup.tile->x == 3);
  REQUIRE(dup.tile->z == 5);
  REQUIRE(dup.y.has_value());
  REQUIRE(*dup.y == Approx(1.5f));
  REQUIRE(dup.volume.has_value());
  REQUIRE(dup.volume->min_x == Approx(3.0f));
  REQUIRE(dup.volume->max_x == Approx(4.0f));
  REQUIRE(dup.pages.size() == 1);
  REQUIRE(dup.pages[0].commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(dup.pages[0].graph.has_value());
  REQUIRE(dup.pages[0].graph->nodes.size() == 1);
  REQUIRE(dup.pages[0].graph->nodes[0].id == "n0");
  REQUIRE(dup.pages[0].conditions.size() == 1);
  REQUIRE(dup.pages[0].conditions[0].id == 3);
}

TEST_CASE("make_duplicate_event skips occupied tiles along +X", "[unit][event_edit]") {
  rat::MapData map;
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  map.events.push_back(rat::make_stub_event("other", 1, 0));
  const rat::EventDef dup = rat::make_duplicate_event(map, 0);
  REQUIRE(dup.tile->x == 2);
  REQUIRE(dup.tile->z == 0);
  REQUIRE(dup.id == "npc_copy");
}

TEST_CASE("duplicate event ids stay unique on the map", "[unit][event_edit]") {
  rat::MapData map;
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  map.events.push_back(rat::make_duplicate_event(map, 0));
  map.events.push_back(rat::make_duplicate_event(map, 0));
  REQUIRE(map.events[1].id == "npc_copy");
  REQUIRE(map.events[2].id == "npc_copy2");
  REQUIRE(map.events[1].id != map.events[0].id);
  REQUIRE(map.events[2].id != map.events[0].id);
  REQUIRE(map.events[2].id != map.events[1].id);
}

TEST_CASE("make_duplicate_event_command undoes via EditHistory",
          "[unit][event_edit][edit_history]") {
  rat::MapData map;
  map.tile_size = 1.0f;
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  rat::EditHistory history;
  REQUIRE(history.execute(map, rat::make_duplicate_event_command(map, 0)));
  REQUIRE(map.events.size() == 2);
  REQUIRE(map.events[1].id == "npc_copy");
  REQUIRE(map.events[1].tile->x == 1);
  REQUIRE(history.undo(map));
  REQUIRE(map.events.size() == 1);
  REQUIRE(map.events[0].id == "npc");
}
