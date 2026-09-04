#include "editor_document.hpp"
#include "event_graph_edit.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_graph.hpp>
#include <rat/map_data.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

[[nodiscard]] rat::MapData make_map_with_event(rat::EventDef event) {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "graph_edit";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);
  map.events.push_back(std::move(event));
  return map;
}

}  // namespace

TEST_CASE("graph place connect delete then apply matches compile_event_graph",
          "[unit][event][edit][graph]") {
  rat::EventGraph graph;
  const std::string say = rat::add_event_graph_node(graph, "show_text");
  REQUIRE_FALSE(say.empty());
  REQUIRE(graph.nodes.size() == 1);
  REQUIRE(graph.nodes[0].kind == "show_text");
  graph.nodes[0].text = "Hello";

  const std::string pause = rat::add_event_graph_node(graph, "wait");
  REQUIRE_FALSE(pause.empty());
  REQUIRE(pause != say);
  graph.nodes[1].frames = 12;

  REQUIRE(rat::connect_event_graph_nodes(graph, rat::kEventGraphEntryId, say));
  REQUIRE(rat::connect_event_graph_nodes(graph, say, pause));
  REQUIRE(rat::connect_event_graph_nodes(graph, pause, rat::kEventGraphExitId));
  REQUIRE(graph.edges.size() == 3);

  REQUIRE(rat::delete_event_graph_node(graph, pause));
  REQUIRE(graph.nodes.size() == 1);
  REQUIRE(rat::connect_event_graph_nodes(graph, say, rat::kEventGraphExitId));

  rat::EventDef event = rat::make_stub_event("npc", 0, 0);
  event.pages[0].commands.clear();
  event.pages[0].graph = graph;

  rat::EditorDocument document;
  document.load(make_map_with_event(event));
  document.select_event(0);
  document.set_selected_page(0);

  const rat::EventGraphCompileResult expected = rat::compile_event_graph(graph);
  REQUIRE(expected.ok);
  REQUIRE(expected.commands.size() == 1);
  REQUIRE(expected.commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(expected.commands[0].text == "Hello");

  const rat::EventGraphApplyResult applied = document.compile_graphs_for_apply();
  REQUIRE(applied.ok);
  REQUIRE(applied.issues.empty());
  REQUIRE(document.data().events[0].pages[0].graph.has_value());
  REQUIRE(document.data().events[0].pages[0].commands.empty());
}

TEST_CASE("invalid graph fails apply and leaves commands unchanged",
          "[unit][event][edit][graph]") {
  rat::EventGraph graph;
  rat::EventGraphNode branch;
  branch.id = "br";
  branch.kind = "conditional_branch";
  branch.branch_condition.type = rat::ConditionType::Switch;
  branch.branch_condition.id = 1;
  branch.branch_condition.bool_value = true;
  graph.nodes.push_back(branch);
  graph.nodes.push_back(rat::EventGraphNode{.id = "no", .kind = "show_text", .text = "Off"});
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "br"},
      rat::EventGraphEdge{.from = "br", .to = "no", .branch = std::string("else")},
      rat::EventGraphEdge{.from = "no", .to = "exit"},
  };

  rat::EventDef event = rat::make_stub_event("npc", 0, 0);
  event.pages[0].commands[0].text = "stale list";
  event.pages[0].graph = graph;

  rat::EditorDocument document;
  document.load(make_map_with_event(event));

  const rat::EventGraphApplyResult applied = document.compile_graphs_for_apply();
  REQUIRE_FALSE(applied.ok);
  REQUIRE_FALSE(applied.issues.empty());
  REQUIRE(rat::map_issues_have_errors(applied.issues));
  REQUIRE(document.data().events[0].pages[0].commands.size() == 1);
  REQUIRE(document.data().events[0].pages[0].commands[0].text == "stale list");
}

TEST_CASE("add_event_graph_node accepts every command kind", "[unit][event][edit][graph][event_edit]") {
  rat::EventGraph graph;
  REQUIRE(rat::add_event_graph_node(graph, "not_a_kind").empty());

  const std::string variable = rat::add_event_graph_node(graph, "control_variable");
  REQUIRE_FALSE(variable.empty());
  REQUIRE(graph.nodes.back().kind == "control_variable");
  REQUIRE(graph.nodes.back().switch_id == 1);
  REQUIRE(graph.nodes.back().int_value == 0);

  const std::string self_switch = rat::add_event_graph_node(graph, "control_self_switch");
  REQUIRE_FALSE(self_switch.empty());
  REQUIRE(graph.nodes.back().kind == "control_self_switch");
  REQUIRE(graph.nodes.back().self_switch == 'A');
  REQUIRE(graph.nodes.back().bool_value == true);

  const std::string transfer = rat::add_event_graph_node(graph, "transfer_player");
  REQUIRE_FALSE(transfer.empty());
  REQUIRE(graph.nodes.back().kind == "transfer_player");
  REQUIRE(graph.nodes.back().map_id == "grey_yard");
  REQUIRE(graph.nodes.back().x == 0.0f);
  REQUIRE(graph.nodes.back().z == 0.0f);

  const std::string items = rat::add_event_graph_node(graph, "change_items");
  REQUIRE_FALSE(items.empty());
  REQUIRE(graph.nodes.back().kind == "change_items");
  REQUIRE(graph.nodes.back().item_id == "item");
  REQUIRE(graph.nodes.back().item_delta == 1);

  const std::string se = rat::add_event_graph_node(graph, "play_se");
  REQUIRE_FALSE(se.empty());
  REQUIRE(graph.nodes.back().kind == "play_se");
  REQUIRE(graph.nodes.back().text == "se");

  const std::string route = rat::add_event_graph_node(graph, "set_move_route");
  REQUIRE_FALSE(route.empty());
  REQUIRE(graph.nodes.back().kind == "set_move_route");
  REQUIRE(graph.nodes.back().route.size() == 1);
  REQUIRE(graph.nodes.back().route[0].op == rat::RouteStepOp::Move);
  REQUIRE(graph.nodes.back().route[0].dir == rat::RampDirection::East);

  const std::string comment = rat::add_event_graph_node(graph, "comment");
  REQUIRE_FALSE(comment.empty());
  REQUIRE(graph.nodes.back().kind == "comment");
  REQUIRE(graph.nodes.back().text == "comment");
}

TEST_CASE("unknown graph kind fails apply", "[unit][event][edit][graph]") {
  rat::EventGraph graph;
  graph.nodes.push_back(rat::EventGraphNode{.id = "n1", .kind = "not_a_kind"});
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "n1"},
      rat::EventGraphEdge{.from = "n1", .to = "exit"},
  };

  rat::EventDef event = rat::make_stub_event("npc", 0, 0);
  event.pages[0].graph = graph;

  rat::EditorDocument document;
  document.load(make_map_with_event(event));
  const rat::EventGraphApplyResult applied = document.compile_graphs_for_apply();
  REQUIRE_FALSE(applied.ok);
  REQUIRE(rat::map_issues_have_errors(applied.issues));
}

TEST_CASE("page without graph keeps inspector command list on apply",
          "[unit][event][edit][graph]") {
  rat::EventDef event = rat::make_stub_event("npc", 0, 0);
  REQUIRE_FALSE(event.pages[0].graph.has_value());
  event.pages[0].commands[0].text = "from inspector";

  rat::EditorDocument document;
  document.load(make_map_with_event(event));
  document.select_event(0);

  rat::EventDef edited = document.data().events[0];
  edited.pages[0].commands[0].text = "edited list";
  REQUIRE(document.execute(rat::make_replace_event_command(0, std::move(edited))).applied);
  REQUIRE_FALSE(document.data().events[0].pages[0].graph.has_value());
  REQUIRE(document.data().events[0].pages[0].commands[0].text == "edited list");

  const rat::EventGraphApplyResult applied = document.compile_graphs_for_apply();
  REQUIRE(applied.ok);
  REQUIRE(applied.issues.empty());
  REQUIRE_FALSE(document.data().events[0].pages[0].graph.has_value());
  REQUIRE(document.data().events[0].pages[0].commands.size() == 1);
  REQUIRE(document.data().events[0].pages[0].commands[0].text == "edited list");
}
