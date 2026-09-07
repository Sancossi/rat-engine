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
  REQUIRE(document.execute(rat::make_replace_event_command(0, std::move(edited))).ok);
  REQUIRE_FALSE(document.data().events[0].pages[0].graph.has_value());
  REQUIRE(document.data().events[0].pages[0].commands[0].text == "edited list");

  const rat::EventGraphApplyResult applied = document.compile_graphs_for_apply();
  REQUIRE(applied.ok);
  REQUIRE(applied.issues.empty());
  REQUIRE_FALSE(document.data().events[0].pages[0].graph.has_value());
  REQUIRE(document.data().events[0].pages[0].commands.size() == 1);
  REQUIRE(document.data().events[0].pages[0].commands[0].text == "edited list");
}

TEST_CASE("duplicate_event_graph_node copies payload and leaves edges unchanged",
          "[unit][event][edit][graph]") {
  rat::EventGraph graph;
  const std::string say = rat::add_event_graph_node(graph, "show_text");
  REQUIRE_FALSE(say.empty());
  graph.nodes[0].text = "Hello copy";
  graph.nodes[0].frames = 7;
  graph.nodes[0].switch_id = 3;
  graph.nodes[0].bool_value = true;
  graph.nodes[0].int_value = 9;
  graph.nodes[0].self_switch = 'B';
  graph.nodes[0].map_id = "yard";
  graph.nodes[0].x = 1.5f;
  graph.nodes[0].item_id = "scrap";
  graph.nodes[0].item_delta = -2;
  graph.nodes[0].key_item = true;
  graph.nodes[0].through = true;
  graph.nodes[0].route.push_back(rat::RouteStep{});

  REQUIRE(rat::connect_event_graph_nodes(graph, rat::kEventGraphEntryId, say));
  REQUIRE(rat::connect_event_graph_nodes(graph, say, rat::kEventGraphExitId));
  const std::size_t edge_count = graph.edges.size();
  REQUIRE(edge_count == 2);

  const std::string copy = rat::duplicate_event_graph_node(graph, say);
  REQUIRE_FALSE(copy.empty());
  REQUIRE(copy != say);
  REQUIRE(graph.nodes.size() == 2);
  REQUIRE(graph.nodes[1].id == copy);
  REQUIRE(graph.nodes[1].kind == "show_text");
  REQUIRE(graph.nodes[1].text == "Hello copy");
  REQUIRE(graph.nodes[1].frames == 7);
  REQUIRE(graph.nodes[1].switch_id == 3);
  REQUIRE(graph.nodes[1].bool_value == true);
  REQUIRE(graph.nodes[1].int_value == 9);
  REQUIRE(graph.nodes[1].self_switch == 'B');
  REQUIRE(graph.nodes[1].map_id == "yard");
  REQUIRE(graph.nodes[1].x == 1.5f);
  REQUIRE(graph.nodes[1].item_id == "scrap");
  REQUIRE(graph.nodes[1].item_delta == -2);
  REQUIRE(graph.nodes[1].key_item == true);
  REQUIRE(graph.nodes[1].through == true);
  REQUIRE(graph.nodes[1].route.size() == 1);
  REQUIRE(graph.edges.size() == edge_count);
  for (const rat::EventGraphEdge& edge : graph.edges) {
    REQUIRE(edge.from != copy);
    REQUIRE(edge.to != copy);
  }
}

TEST_CASE("duplicate_event_graph_node refuses entry exit and missing",
          "[unit][event][edit][graph]") {
  rat::EventGraph graph;
  const std::string say = rat::add_event_graph_node(graph, "show_text");
  REQUIRE_FALSE(say.empty());
  REQUIRE(rat::duplicate_event_graph_node(graph, rat::kEventGraphEntryId).empty());
  REQUIRE(rat::duplicate_event_graph_node(graph, rat::kEventGraphExitId).empty());
  REQUIRE(rat::duplicate_event_graph_node(graph, "missing").empty());
  REQUIRE(rat::duplicate_event_graph_node(graph, "").empty());
  REQUIRE(graph.nodes.size() == 1);
  REQUIRE(graph.edges.empty());
}

TEST_CASE("event_graph_pin_contains is a circle independent of submit order",
          "[unit][event][edit][graph]") {
  REQUIRE(rat::event_graph_pin_contains(10.0f, 20.0f, 7.0f, 10.0f, 20.0f));
  REQUIRE(rat::event_graph_pin_contains(10.0f, 20.0f, 7.0f, 16.0f, 20.0f));
  REQUIRE_FALSE(rat::event_graph_pin_contains(10.0f, 20.0f, 7.0f, 18.0f, 20.0f));
  REQUIRE_FALSE(rat::event_graph_pin_contains(10.0f, 20.0f, 7.0f, 16.0f, 26.0f));
  REQUIRE_FALSE(rat::event_graph_pin_contains(10.0f, 20.0f, 0.0f, 10.0f, 20.0f));
}

TEST_CASE("event_graph_canvas_history_action maps Ctrl+Z/Y chords",
          "[unit][event][edit][graph]") {
  using rat::EventGraphCanvasHistoryAction;
  REQUIRE(rat::event_graph_canvas_history_action(true, false, true, false) ==
          EventGraphCanvasHistoryAction::Undo);
  REQUIRE(rat::event_graph_canvas_history_action(true, false, false, true) ==
          EventGraphCanvasHistoryAction::Redo);
  REQUIRE(rat::event_graph_canvas_history_action(true, true, true, false) ==
          EventGraphCanvasHistoryAction::Redo);
  REQUIRE(rat::event_graph_canvas_history_action(false, false, true, false) ==
          EventGraphCanvasHistoryAction::None);
  REQUIRE(rat::event_graph_canvas_history_action(true, false, false, false) ==
          EventGraphCanvasHistoryAction::None);
  REQUIRE(rat::event_graph_canvas_history_action(true, true, false, false) ==
          EventGraphCanvasHistoryAction::None);
}

TEST_CASE("Completed graph wire gestures clear rejected and empty connections", "[unit][event][edit][graph]") {
  for (const std::string target : {"n", "missing", ""}) {
    rat::EventGraph graph; (void)rat::add_event_graph_node(graph,"show_text");
    graph.nodes[0].id="n";
    rat::EventGraphWireState wire;wire.pending_from="n";wire.pending_branch="then";
    wire.dragging_wire=true;wire.drag_wire_from="n";wire.drag_wire_branch="then";wire.wire_moved=true;
    const auto source=wire.drag_wire_from;const auto branch=wire.drag_wire_branch;
    rat::finish_event_graph_wire_release(wire,false);
    CHECK_FALSE(wire.dragging_wire);CHECK(wire.drag_wire_from.empty());CHECK_FALSE(wire.drag_wire_branch);
    CHECK(wire.pending_from.empty());CHECK_FALSE(wire.pending_branch);
    if(!target.empty()) CHECK_FALSE(rat::connect_event_graph_nodes(graph,source,target,branch));
    CHECK(graph.edges.empty());
  }
  rat::EventGraphWireState click;click.pending_from="n";click.dragging_wire=true;click.drag_wire_from="n";
  rat::finish_event_graph_wire_release(click,true);
  CHECK_FALSE(click.dragging_wire);CHECK(click.pending_from=="n");
  rat::EventGraph graph;auto node=rat::add_event_graph_node(graph,"comment");
  CHECK(rat::connect_event_graph_nodes(graph,node,rat::kEventGraphExitId));
  rat::finish_event_graph_wire_release(click,false);CHECK(click.pending_from.empty());
}
