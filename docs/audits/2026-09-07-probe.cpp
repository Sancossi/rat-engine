#include <rat/game_state.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>
#include <rat/event_edit.hpp>
#include <rat/replay.hpp>
#include <rat/surface_query.hpp>
#include "editor_document.hpp"
#include <cmath>
#include <iostream>
#include <limits>

int main() {
  using namespace rat;
  GameState s;
  std::cout << "header_only_save_accepted=" << s.load_from_memory("RATSAVE1\n") << '\n';
  std::cout << "nan_save_accepted=" << s.load_from_memory("RATSAVE1\nmap audit\npos nan 0 0\n") << " x_is_nan=" << std::isnan(s.player_x()) << '\n';
  s.clear(); s.set_map_id("audit"); s.add_item("rusty cog", 1);
  std::string blob; (void)s.save_to_memory(blob);
  GameState restored;
  std::cout << "spaced_item_save_roundtrip=" << restored.load_from_memory(blob) << '\n';
  MapData map; map.id="audit"; map.schema_version=5; map.width=2; map.height=2;
  map.height_grid.width=2; map.height_grid.height=2; map.height_grid.ground_y.assign(4, 0.f);
  std::cout << "baseline_map_ok=" << compile_map_data(map).ok << '\n';
  auto nanmap=map; nanmap.tile_size=std::numeric_limits<float>::quiet_NaN();
  const auto serialized=serialize_map_to_string(nanmap);
  std::cout << "nan_tile_compile_ok=" << compile_map_data(nanmap).ok << " serialize_ok=" << serialized.ok << " reload_ok=" << load_map_document_from_string(serialized.json_text).ok << '\n';
  EditorDocument doc; map.events.push_back(make_stub_event("stub_1",0,0)); doc.load(map);
  auto placed=doc.execute(make_place_event_command(make_stub_event("stub_1",1,0)));
  std::cout << "duplicate_event_placed=" << placed.applied << " compile_ok=" << compile_map_data(doc.data()).ok << '\n';
  (void)doc.undo(); doc.mark_clean();
  (void)doc.execute(make_move_event_command(0,1,0,1.f)); (void)doc.undo();
  std::cout << "undo_to_saved_still_dirty=" << doc.dirty() << '\n';
  doc.load(map); (void)doc.compile_graphs_for_apply(); doc.mark_clean();
  (void)doc.compile_graphs_for_apply();
  std::cout << "no_change_compile_marks_dirty=" << doc.dirty() << '\n';
  MemoryFileStore files; (void)files.write("r", "{}");
  std::cout << "empty_replay_accepted=" << read_replay("r",files).has_value() << '\n';
  (void)files.write("r", "{\"schema_version\":999,\"ticks\":[]}");
  std::cout << "future_replay_accepted=" << read_replay("r",files).has_value() << '\n';
  const auto routeMap=[](const char* dir) {
    return load_map_from_string(std::string(R"({"schema_version":1,"id":"route","width":8,"height":8,"events":[{"id":"npc","tile":{"x":2,"z":2},"pages":[{"trigger":"parallel","commands":[{"op":"set_move_route","route":[{"op":"move","dir":")")+dir+R"("}]}]}]}]})").map;
  };
  SimulationSession east, west;
  std::cout << "route_maps_ok=" << (east.load(routeMap("east")).ok && west.load(routeMap("west")).ok) << '\n';
  east.tick({}); west.tick({});
  const auto e=east.events().event_overlay("npc"), w=west.events().event_overlay("npc");
  std::cout << "npc_positions_differ=" << (e && w && e->x != w->x) << " npc_checksum_equal=" << (runtime_checksum(east,0)==runtime_checksum(west,0)) << '\n';
  const auto transfer=load_map_from_string(R"({"schema_version":1,"id":"from","width":8,"height":8,"events":[{"id":"door","tile":{"x":0,"z":0},"pages":[{"trigger":"autorun","commands":[{"op":"transfer_player","map_id":"other","x":1,"y":0,"z":1}]}]}]})");
  SimulationSession travel; (void)travel.load(transfer.map); travel.tick({});
  std::cout << "transfer_state_map=" << travel.state().map_id() << " runtime_map=" << travel.events().map().id << '\n';
}
