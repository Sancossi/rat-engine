#include <rat/event_graph.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

[[nodiscard]] std::string map_json_with_page(std::string_view page_json) {
  std::string json;
  json += R"({
    "schema_version": 1,
    "id": "graph_map",
    "width": 2,
    "height": 2,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [)";
  json += page_json;
  json += R"(]
      }
    ]
  })";
  return json;
}

[[nodiscard]] bool has_error_mentioning(const std::vector<rat::MapIssue>& issues,
                                        std::string_view needle) {
  for (const rat::MapIssue& issue : issues) {
    if (issue.severity == rat::MapIssueSeverity::Error &&
        issue.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_CASE("compile linear show_text then wait", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "say", .kind = "show_text", .text = "Hello"},
      rat::EventGraphNode{.id = "pause", .kind = "wait", .frames = 12},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "say"},
      rat::EventGraphEdge{.from = "say", .to = "pause"},
      rat::EventGraphEdge{.from = "pause", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.issues.empty());
  REQUIRE(compiled.commands.size() == 2);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(compiled.commands[0].text == "Hello");
  REQUIRE(compiled.commands[1].op == rat::CommandOp::Wait);
  REQUIRE(compiled.commands[1].frames == 12);
}

TEST_CASE("compile control_switch node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{
          .id = "sw", .kind = "control_switch", .switch_id = 7, .bool_value = true},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "sw"},
      rat::EventGraphEdge{.from = "sw", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ControlSwitch);
  REQUIRE(compiled.commands[0].id == 7);
  REQUIRE(compiled.commands[0].bool_value == true);
}

TEST_CASE("compile conditional_branch then and else", "[unit][event][graph]") {
  rat::EventGraph graph;
  rat::EventGraphNode branch;
  branch.id = "br";
  branch.kind = "conditional_branch";
  branch.branch_condition.type = rat::ConditionType::Switch;
  branch.branch_condition.id = 3;
  branch.branch_condition.bool_value = true;
  graph.nodes = {
      branch,
      rat::EventGraphNode{.id = "yes", .kind = "show_text", .text = "On"},
      rat::EventGraphNode{.id = "no", .kind = "show_text", .text = "Off"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "br"},
      rat::EventGraphEdge{.from = "br", .to = "yes", .branch = std::string("then")},
      rat::EventGraphEdge{.from = "br", .to = "no", .branch = std::string("else")},
      rat::EventGraphEdge{.from = "yes", .to = "exit"},
      rat::EventGraphEdge{.from = "no", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ConditionalBranch);
  REQUIRE(compiled.commands[0].branch_condition.type == rat::ConditionType::Switch);
  REQUIRE(compiled.commands[0].branch_condition.id == 3);
  REQUIRE(compiled.commands[0].branch_condition.bool_value == true);
  REQUIRE(compiled.commands[0].then_commands.size() == 1);
  REQUIRE(compiled.commands[0].then_commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(compiled.commands[0].then_commands[0].text == "On");
  REQUIRE(compiled.commands[0].else_commands.size() == 1);
  REQUIRE(compiled.commands[0].else_commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(compiled.commands[0].else_commands[0].text == "Off");
}

TEST_CASE("sibling edges without order sort by target id", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "b", .kind = "show_text", .text = "B"},
      rat::EventGraphNode{.id = "a", .kind = "show_text", .text = "A"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "b"},
      rat::EventGraphEdge{.from = "entry", .to = "a"},
      rat::EventGraphEdge{.from = "a", .to = "exit"},
      rat::EventGraphEdge{.from = "b", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 2);
  REQUIRE(compiled.commands[0].text == "A");
  REQUIRE(compiled.commands[1].text == "B");
}

TEST_CASE("sibling edges honor explicit order", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "a", .kind = "show_text", .text = "A"},
      rat::EventGraphNode{.id = "b", .kind = "show_text", .text = "B"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "b", .order = 0},
      rat::EventGraphEdge{.from = "entry", .to = "a", .order = 1},
      rat::EventGraphEdge{.from = "a", .to = "exit"},
      rat::EventGraphEdge{.from = "b", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 2);
  REQUIRE(compiled.commands[0].text == "B");
  REQUIRE(compiled.commands[1].text == "A");
}

TEST_CASE("map JSON graph round-trips through serialize/load", "[unit][event][graph][map]") {
  constexpr const char* kPage = R"({
    "trigger": "action",
    "commands": [
      { "op": "show_text", "text": "Hello" },
      { "op": "wait", "frames": 8 }
    ],
    "graph": {
      "nodes": [
        { "id": "say", "kind": "show_text", "params": { "text": "Hello" } },
        { "id": "pause", "kind": "wait", "params": { "frames": 8 } }
      ],
      "edges": [
        { "from": "entry", "to": "say" },
        { "from": "say", "to": "pause" },
        { "from": "pause", "to": "exit" }
      ]
    }
  })";

  const std::string json = map_json_with_page(kPage);
  const rat::MapLoadResult loaded = rat::load_map_from_string(json);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events.size() == 1);
  REQUIRE(loaded.map.events[0].pages.size() == 1);
  const rat::EventPage& page = loaded.map.events[0].pages[0];
  REQUIRE(page.commands.size() == 2);
  REQUIRE(page.graph.has_value());
  REQUIRE(page.graph->nodes.size() == 2);
  REQUIRE(page.graph->nodes[0].id == "say");
  REQUIRE(page.graph->nodes[0].kind == "show_text");
  REQUIRE(page.graph->nodes[0].text == "Hello");
  REQUIRE(page.graph->nodes[1].id == "pause");
  REQUIRE(page.graph->nodes[1].kind == "wait");
  REQUIRE(page.graph->nodes[1].frames == 8);
  REQUIRE(page.graph->edges.size() == 3);
  REQUIRE(page.graph->edges[0].from == "entry");
  REQUIRE(page.graph->edges[0].to == "say");

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(*page.graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 2);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(compiled.commands[0].text == "Hello");
  REQUIRE(compiled.commands[1].op == rat::CommandOp::Wait);
  REQUIRE(compiled.commands[1].frames == 8);

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const rat::MapLoadResult again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events[0].pages[0].graph.has_value());
  REQUIRE(again.map.events[0].pages[0].graph->nodes.size() == 2);
  REQUIRE(again.map.events[0].pages[0].graph->nodes[0].text == "Hello");
  REQUIRE(again.map.events[0].pages[0].graph->nodes[1].frames == 8);
  REQUIRE(again.map.events[0].pages[0].graph->edges.size() == 3);
  REQUIRE(again.map.events[0].pages[0].commands[0].text == "Hello");
}

TEST_CASE("page without graph still loads commands", "[unit][event][graph][map]") {
  constexpr const char* kPage = R"({
    "trigger": "action",
    "commands": [ { "op": "show_text", "text": "No graph" } ]
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);
  REQUIRE_FALSE(loaded.map.events[0].pages[0].graph.has_value());
  REQUIRE(loaded.map.events[0].pages[0].commands.size() == 1);
  REQUIRE(loaded.map.events[0].pages[0].commands[0].text == "No graph");

  const rat::MapDocumentLoadResult doc = rat::load_map_document_from_string(map_json_with_page(kPage));
  REQUIRE(doc.ok);
  REQUIRE(rat::compile_map_data(loaded.map).ok);
}

TEST_CASE("invalid graph fails validate and document load", "[unit][event][graph][map]") {
  SECTION("unknown kind") {
    constexpr const char* kPage = R"({
      "trigger": "action",
      "commands": [],
      "graph": {
        "nodes": [ { "id": "n1", "kind": "play_se", "params": { "id": "beep" } } ],
        "edges": [ { "from": "entry", "to": "n1" }, { "from": "n1", "to": "exit" } ]
      }
    })";
    const std::string json = map_json_with_page(kPage);
    const rat::MapLoadResult loaded = rat::load_map_from_string(json);
    REQUIRE(loaded.ok);
    const std::vector<rat::MapIssue> issues = rat::validate_map_document(loaded.map);
    REQUIRE(rat::map_issues_have_errors(issues));
    REQUIRE(has_error_mentioning(issues, "unknown"));
    REQUIRE_FALSE(rat::compile_map_data(loaded.map).ok);
    const rat::MapDocumentLoadResult doc = rat::load_map_document_from_string(json);
    REQUIRE_FALSE(doc.ok);
    REQUIRE(rat::map_issues_have_errors(doc.issues));
  }

  SECTION("cycle without Wait") {
    constexpr const char* kPage = R"({
      "trigger": "action",
      "commands": [],
      "graph": {
        "nodes": [
          { "id": "a", "kind": "show_text", "params": { "text": "A" } },
          { "id": "b", "kind": "show_text", "params": { "text": "B" } }
        ],
        "edges": [
          { "from": "entry", "to": "a" },
          { "from": "a", "to": "b" },
          { "from": "b", "to": "a" }
        ]
      }
    })";
    const std::string json = map_json_with_page(kPage);
    const rat::MapLoadResult loaded = rat::load_map_from_string(json);
    REQUIRE(loaded.ok);
    REQUIRE(rat::map_issues_have_errors(rat::validate_map_document(loaded.map)));
    REQUIRE(has_error_mentioning(rat::validate_map_document(loaded.map), "cycle"));
    REQUIRE_FALSE(rat::load_map_document_from_string(json).ok);
  }

  SECTION("unreachable node") {
    constexpr const char* kPage = R"({
      "trigger": "action",
      "commands": [],
      "graph": {
        "nodes": [
          { "id": "used", "kind": "wait", "params": { "frames": 1 } },
          { "id": "orphan", "kind": "show_text", "params": { "text": "lost" } }
        ],
        "edges": [
          { "from": "entry", "to": "used" },
          { "from": "used", "to": "exit" }
        ]
      }
    })";
    const std::string json = map_json_with_page(kPage);
    const rat::MapLoadResult loaded = rat::load_map_from_string(json);
    REQUIRE(loaded.ok);
    REQUIRE(has_error_mentioning(rat::validate_map_document(loaded.map), "unreachable"));
    REQUIRE_FALSE(rat::load_map_document_from_string(json).ok);
  }

  SECTION("branch without then-edge") {
    constexpr const char* kPage = R"({
      "trigger": "action",
      "commands": [],
      "graph": {
        "nodes": [
          { "id": "br", "kind": "conditional_branch",
            "params": { "condition": { "type": "switch", "id": 1, "value": true } } },
          { "id": "no", "kind": "show_text", "params": { "text": "Off" } }
        ],
        "edges": [
          { "from": "entry", "to": "br" },
          { "from": "br", "to": "no", "branch": "else" },
          { "from": "no", "to": "exit" }
        ]
      }
    })";
    const std::string json = map_json_with_page(kPage);
    const rat::MapLoadResult loaded = rat::load_map_from_string(json);
    REQUIRE(loaded.ok);
    REQUIRE(has_error_mentioning(rat::validate_map_document(loaded.map), "then"));
    REQUIRE_FALSE(rat::load_map_document_from_string(json).ok);
  }
}

TEST_CASE("EventRuntime uses commands not graph", "[unit][event][graph]") {
  constexpr const char* kPage = R"({
    "trigger": "autorun",
    "commands": [ { "op": "show_text", "text": "FromCommands" } ],
    "graph": {
      "nodes": [ { "id": "say", "kind": "show_text", "params": { "text": "FromGraph" } } ],
      "edges": [ { "from": "entry", "to": "say" }, { "from": "say", "to": "exit" } ]
    }
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "FromCommands");
}
