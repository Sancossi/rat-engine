#include "quantiles.hpp"
#include "editor_document.hpp"
#include <rat/collision.hpp>
#include <rat/hot_apply.hpp>
#include <rat/map_loader.hpp>
#include <rat/simulation_session.hpp>
#include <rat/terrain_geometry.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
constexpr int warmup = 1000;
constexpr int samples = 10000;
constexpr int bake_samples = 100;
std::uint64_t observed = 0;  // Results consumed in report; prevent dead-work elimination.
template<class F> double measure(F&& fn) {
  const auto start = Clock::now(); fn();
  return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
}
Json summary(const std::vector<double>& values) {
  return {{"samples", values.size()}, {"unit", "microseconds"},
      {"p50", rat::benchmark::quantile(values,.5)},
      {"p95", rat::benchmark::quantile(values,.95)},
      {"p99", rat::benchmark::quantile(values,.99)}};
}
rat::MapData synthetic(const std::string& name, int side, int count) {
  rat::MapData map; map.schema_version = 5; map.id = name;
  map.width = map.height = side;
  map.height_grid = {0,0,side,side,std::vector<float>(static_cast<std::size_t>(side)*side,0)};
  for (int i=0; i<count; ++i) {
    const int x=2+(i*7)%(side-4), z=2+(i*11)%(side-4);
    map.blockers.push_back({{static_cast<float>(x),static_cast<float>(z),
        static_cast<float>(x)+.5f,static_cast<float>(z)+.5f}});
    rat::EventDef event; event.id="npc_"+std::to_string(i); event.tile=rat::TileCoord{x,z};
    rat::EventPage page; page.trigger=rat::TriggerKind::Parallel;
    rat::Command wait; wait.op=rat::CommandOp::Wait; wait.frames=30;
    page.commands.push_back(wait); event.pages.push_back(page); map.events.push_back(event);
  }
  // Unique cells on a sparse raised row; away from the initial player.
  for (int x=2; x<side-2; x+=2) map.occupancy.push_back({x,0,side-2});
  return map;
}
Json memory(const rat::EditorDocument& doc, const std::string& phase) {
  const auto m=doc.estimated_retained_memory(); const auto& h=m.history;
  return {{"phase",phase},{"unit","estimated_retained_bytes"},
      {"history",{{"total",h.total_bytes()},{"object",h.object_bytes},
        {"queue_capacity",h.queue_capacity_bytes},{"undo_commands",h.undo_commands_bytes},
        {"redo_commands",h.redo_commands_bytes},{"stroke_commands",h.stroke_commands_bytes},
        {"stroke_buffers",h.stroke_buffers_bytes}}},
      {"document_other",{{"object",m.document_object_bytes},{"map_dynamic",m.map_dynamic_bytes},
        {"preview_dynamic",m.preview_dynamic_bytes},{"clean_snapshot",m.clean_snapshot_bytes},
        {"error",m.error_bytes}}},{"combined_total",m.total_bytes()}};
}
void require_edit(const rat::EditApplyResult& result) {
  if (!result.ok || !result.changed) throw std::runtime_error("benchmark edit did not change map: "+result.error);
}
Json edit_memory(const rat::MapData& map) {
  rat::EditorDocument doc; doc.load(map); Json result=Json::array({memory(doc,"loaded")});
  const auto x=map.height_grid.origin_x, z=map.height_grid.origin_z;
  for (int i=0;i<100;++i) require_edit(doc.execute(rat::make_set_map_tile_ground_y_command(x,z,static_cast<float>(i+1))));
  result.push_back(memory(doc,"100_terrain_edits"));
  rat::EventDef event; event.id="benchmark_added_event"; event.tile=rat::TileCoord{x,z}; event.pages.resize(1);
  require_edit(doc.execute(rat::make_place_event_command(event)));
  const auto index=doc.data().events.size()-1;
  for (int i=0;i<100;++i) {
    event=doc.data().events[index]; event.pages[0].graph.emplace();
    rat::EventGraphNode node; node.id="comment"; node.kind="comment"; node.text=std::string(256,'a')+std::to_string(i);
    event.pages[0].graph->nodes.push_back(node);
    require_edit(doc.execute(rat::make_replace_event_command(index,event)));
  }
  result.push_back(memory(doc,"100_graph_edits_plus_event"));
  for (int i=0;i<50;++i) require_edit(doc.undo());
  result.push_back(memory(doc,"undo_50"));
  for (int i=0;i<25;++i) require_edit(doc.redo());
  result.push_back(memory(doc,"redo_25"));
  doc.begin_stroke();
  for (int i=0;i<10;++i) require_edit(doc.execute(rat::make_move_event_command(index,1,0,map.tile_size)));
  result.push_back(memory(doc,"active_stroke_10")); doc.end_stroke();
  result.push_back(memory(doc,"committed_stroke"));
  return result;
}
Json run(const rat::MapData& raw) {
  auto compiled=rat::compile_map_data(raw);
  if (!compiled.ok) throw std::runtime_error(rat::format_map_issues(compiled.issues));
  const auto& map=compiled.runtime.data;
  rat::SimulationSession session;
  if (!session.load(map).ok) throw std::runtime_error("session load failed");
  rat::PlayerBody player; player.x=static_cast<float>(map.height_grid.origin_x)+.5f;
  player.z=static_cast<float>(map.height_grid.origin_z)+.5f; session.set_player(player);
  std::vector<double> ticks; ticks.reserve(samples);
  for (int i=0;i<warmup+samples;++i) {
    rat::InputFrame input; input.move.axis_x=(i/120)%2 ? -.25f : .25f;
    input.jump_pressed=i%240==0; input.jump_held=i%240<20;
    const auto elapsed=measure([&]{observed+=session.tick(input).tick_id;});
    if(i>=warmup) ticks.push_back(elapsed);
  }
  std::vector<double> terrain, collision, apply, load;
  rat::SurfaceQuery query(map);
  for(int i=0;i<bake_samples+10;++i) {
    const auto t=measure([&]{observed+=rat::build_terrain_geometry(map.height_grid,map.ramps,map.tile_size).tiles.size();});
    const auto c=measure([&]{const auto world=rat::bake_collision_world(map,query); observed+=world.boxes.size()+world.fences.size();});
    rat::SimulationSession fresh;
    const auto l=measure([&]{if(!fresh.load(map).ok) throw std::runtime_error("load failed");});
    rat::EventRuntime events; rat::GameState state; rat::PlayerBody body;
    std::vector<rat::BlockerDef> blockers; std::vector<rat::Vec3> markers;
    std::unique_ptr<rat::SurfaceQuery> surface;
    rat::HotApplyTargets targets{events,state,body,blockers,markers,&surface};
    const auto a=measure([&]{if(!rat::hot_apply_map(map,targets).ok) throw std::runtime_error("apply failed");});
    observed+=blockers.size()+markers.size();
    if(i>=10) {terrain.push_back(t);collision.push_back(c);apply.push_back(a);load.push_back(l);}
  }
  return {{"map",map.id},{"width",map.width},{"height",map.height},
    {"events",map.events.size()},{"blockers",map.blockers.size()},{"occupancy",map.occupancy.size()},
    {"tick",summary(ticks)},{"terrain_bake",summary(terrain)},{"collision_bake",summary(collision)},
    {"hot_apply_fresh_targets",summary(apply)},{"session_load_fresh",summary(load)},
    {"edit_memory",edit_memory(map)}};
}
}  // namespace
int main(int argc,char** argv) {
  try {
    if(argc!=2) throw std::runtime_error("usage: rat-benchmark PATH_TO_GREY_YARD_JSON");
    const auto grey=rat::load_map_from_file(argv[1]); if(!grey.ok) throw std::runtime_error(grey.error);
    Json report={{"schema_version",1},{"compiler",RAT_BENCHMARK_COMPILER},{"build_type",RAT_BENCHMARK_BUILD_TYPE},
      {"clock","std::chrono::steady_clock"},{"quantile","nearest-rank ceil(p*N)-1"},
      {"tick_warmup",warmup},{"tick_samples",samples},{"dt_seconds",rat::kSimulationFixedDt},
      {"bake_apply_warmup",10},{"bake_apply_samples",bake_samples},
      {"memory_exclusions",{"allocator overhead","opaque std::function target storage","temporary work allocations","unowned services"}},
      {"maps",Json::array()}};
    report["maps"].push_back(run(grey.map));
    report["maps"].push_back(run(synthetic("synthetic_small",16,8)));
    report["maps"].push_back(run(synthetic("synthetic_medium",32,32)));
    report["maps"].push_back(run(synthetic("synthetic_large",64,128)));
    report["observed_result_sum"]=observed;
    std::cout<<report.dump(2)<<'\n'; return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
