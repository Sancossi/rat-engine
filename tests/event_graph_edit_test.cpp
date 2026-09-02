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

[[nodiscard]] bool commands_match(const std::vector<rat::Command>& a,
                                  const std::vector<rat::Command>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    const rat::Command& left = a[i];
    const rat::Command& right = b[i];
    if (left.op != right.op || left.text != right.text || left.id != right.id ||
        left.bool_value != right.bool_value || left.frames != right.frames ||
        left.branch_condition.type != right.branch_condition.type ||
        left.branch_condition.id != right.branch_condition.id ||
        left.branch_condition.bool_value != right.branch_condition.bool_value) {
      return false;
    }
    if (!commands_match(left.then_commands, right.then_commands) ||
        !commands_match(left.else_commands, right.else_commands)) {
      return false;
    }
  }
  return true;
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

  const rat::EventGraphApplyResult applied = document.compile_graphs_for_apply();
  REQUIRE(applied.ok);
  REQUIRE(applied.issues.empty());
  REQUIRE(document.data().events[0].pages[0].graph.has_value());
  REQUIRE(commands_match(document.data().events[0].pages[0].commands, expected.commands));
  REQUIRE(document.data().events[0].pages[0].commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(document.data().events[0].pages[0].commands[0].text == "Hello");
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

TEST_CASE("unknown graph kind fails apply", "[unit][event][edit][graph]") {
  rat::EventGraph graph;
  graph.nodes.push_back(rat::EventGraphNode{.id = "n1", .kind = "play_se"});
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
