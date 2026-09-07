#include <rat/retained_memory.hpp>
#include <rat/edit_history.hpp>
#include "editor_document.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Retained map memory includes unused capacity and nested graph and legacy allocations", "[memory]") {
  rat::MapData map;
  const auto before = rat::retained_dynamic_bytes(map);
  map.occupancy.reserve(100);
  REQUIRE(rat::retained_dynamic_bytes(map) - before == map.occupancy.capacity() * sizeof(rat::OccupancyCell));
  map.events.resize(1); map.events[0].pages.resize(1);
  auto& page = map.events[0].pages[0];
  page.commands.resize(1); page.commands[0].then_commands.resize(1);
  auto baseline = rat::retained_dynamic_bytes(map);
  auto& text = page.commands[0].then_commands[0].text;
  const auto old_text = rat::retained_dynamic_bytes(text);
  text.reserve(4096);
  REQUIRE(rat::retained_dynamic_bytes(map) - baseline == rat::retained_dynamic_bytes(text) - old_text);
  page.graph.emplace(); page.graph->nodes.resize(1); page.graph->edges.resize(1);
  baseline = rat::retained_dynamic_bytes(map);
  page.graph->nodes[0].route.reserve(123);
  REQUIRE(rat::retained_dynamic_bytes(map) - baseline == page.graph->nodes[0].route.capacity() * sizeof(rat::RouteStep));
  page.graph->edges[0].branch.emplace(500, 'x');
  REQUIRE(rat::retained_dynamic_bytes(map) >= baseline + 501);
}

TEST_CASE("History memory follows commands through undo redo composite and clear", "[memory]") {
  rat::MapData map; map.id = "memory"; map.width = map.height = 4;
  rat::EditHistory history;
  rat::EventDef event; event.id = "event"; event.tile = rat::TileCoord{0,0}; event.pages.resize(1);
  event.pages[0].commands.resize(1); event.pages[0].commands[0].text.assign(4096, 'a');
  REQUIRE(history.execute(map, rat::make_place_event_command(event)).changed);
  const auto placed = history.estimated_retained_memory();
  REQUIRE(placed.undo_commands_bytes > 4096);
  REQUIRE(history.undo(map).changed);
  const auto undone = history.estimated_retained_memory();
  REQUIRE(undone.undo_commands_bytes == 0);
  REQUIRE(undone.redo_commands_bytes == placed.undo_commands_bytes);
  REQUIRE(history.redo(map).changed);
  history.begin_stroke();
  REQUIRE(history.execute(map, rat::make_move_event_command(0, 1, 0, 1)).changed);
  REQUIRE(history.execute(map, rat::make_move_event_command(0, 1, 0, 1)).changed);
  const auto stroke = history.estimated_retained_memory();
  REQUIRE(stroke.stroke_commands_bytes > 0);
  REQUIRE(stroke.stroke_buffers_bytes > 8192);
  history.end_stroke();
  const auto composite = history.estimated_retained_memory();
  REQUIRE(composite.stroke_commands_bytes == 0);
  REQUIRE(composite.undo_commands_bytes > placed.undo_commands_bytes + stroke.stroke_commands_bytes);
  history.clear();
  const auto cleared = history.estimated_retained_memory();
  REQUIRE(cleared.undo_commands_bytes == 0);
  REQUIRE(cleared.redo_commands_bytes == 0);
  REQUIRE(cleared.queue_capacity_bytes == composite.queue_capacity_bytes);
  REQUIRE(cleared.stroke_buffers_bytes == composite.stroke_buffers_bytes);
}

TEST_CASE("Document memory separates preview and clean state from history", "[memory]") {
  rat::MapData map; map.id = "memory"; map.width = map.height = 4;
  map.height_grid = {0, 0, 4, 4, std::vector<float>(16, 0)};
  rat::EventDef event; event.id = "event"; event.tile = rat::TileCoord{0,0};
  map.events.push_back(event);
  rat::EditorDocument doc; doc.load(map);
  const auto clean = doc.estimated_retained_memory();
  REQUIRE(clean.clean_snapshot_bytes > 0);
  event.id.assign(200, 'b');
  REQUIRE(doc.preview_event(0, event));
  const auto preview = doc.estimated_retained_memory();
  REQUIRE(preview.preview_dynamic_bytes > 200);
  REQUIRE(preview.history.total_bytes() == clean.history.total_bytes());
  doc.discard_preview();
  REQUIRE(doc.estimated_retained_memory().preview_dynamic_bytes == 0);
  REQUIRE(doc.execute(rat::make_set_map_tile_ground_y_command(0,0,1)).changed);
  REQUIRE(doc.estimated_retained_memory().history.undo_commands_bytes >= 2 * 16 * sizeof(float));
  const auto edited = doc.estimated_retained_memory();
  REQUIRE_FALSE(doc.execute(rat::make_set_map_tile_ground_y_command(0,0,1)).changed);
  REQUIRE(doc.estimated_retained_memory().history.total_bytes() == edited.history.total_bytes());
}
