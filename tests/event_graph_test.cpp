#include <rat/event_graph.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <vector>

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

void require_commands_equal(const std::vector<rat::Command>& actual,
                            const std::vector<rat::Command>& expected) {
  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    REQUIRE(actual[i].op == expected[i].op);
    REQUIRE(actual[i].text == expected[i].text);
    REQUIRE(actual[i].id == expected[i].id);
    REQUIRE(actual[i].bool_value == expected[i].bool_value);
    REQUIRE(actual[i].int_value == expected[i].int_value);
    REQUIRE(actual[i].frames == expected[i].frames);
    REQUIRE(actual[i].through == expected[i].through);
    REQUIRE(actual[i].branch_condition.type == expected[i].branch_condition.type);
    REQUIRE(actual[i].branch_condition.id == expected[i].branch_condition.id);
    REQUIRE(actual[i].branch_condition.bool_value == expected[i].branch_condition.bool_value);
    REQUIRE(actual[i].route.size() == expected[i].route.size());
    for (std::size_t r = 0; r < expected[i].route.size(); ++r) {
      REQUIRE(actual[i].route[r].op == expected[i].route[r].op);
      REQUIRE(actual[i].route[r].dir == expected[i].route[r].dir);
      REQUIRE(actual[i].route[r].frames == expected[i].route[r].frames);
    }
    require_commands_equal(actual[i].then_commands, expected[i].then_commands);
    require_commands_equal(actual[i].else_commands, expected[i].else_commands);
  }
}

void require_round_trip(const std::vector<rat::Command>& cmds) {
  const rat::EventGraph graph = rat::commands_to_graph(cmds);
  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  require_commands_equal(compiled.commands, cmds);
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

TEST_CASE("compile join after then and else reconverge", "[unit][event][graph]") {
  rat::EventGraph graph;
  rat::EventGraphNode branch;
  branch.id = "br";
  branch.kind = "conditional_branch";
  branch.branch_condition.type = rat::ConditionType::Switch;
  branch.branch_condition.id = 2;
  branch.branch_condition.bool_value = true;
  graph.nodes = {
      branch,
      rat::EventGraphNode{.id = "yes", .kind = "show_text", .text = "Then"},
      rat::EventGraphNode{.id = "no", .kind = "show_text", .text = "Else"},
      rat::EventGraphNode{.id = "after", .kind = "wait", .frames = 9},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "br"},
      rat::EventGraphEdge{.from = "br", .to = "yes", .branch = std::string("then")},
      rat::EventGraphEdge{.from = "br", .to = "no", .branch = std::string("else")},
      rat::EventGraphEdge{.from = "yes", .to = "after"},
      rat::EventGraphEdge{.from = "no", .to = "after"},
      rat::EventGraphEdge{.from = "after", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.issues.empty());
  REQUIRE(compiled.commands.size() == 2);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ConditionalBranch);
  REQUIRE(compiled.commands[0].then_commands.size() == 1);
  REQUIRE(compiled.commands[0].then_commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(compiled.commands[0].then_commands[0].text == "Then");
  REQUIRE(compiled.commands[0].else_commands.size() == 1);
  REQUIRE(compiled.commands[0].else_commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(compiled.commands[0].else_commands[0].text == "Else");
  REQUIRE(compiled.commands[1].op == rat::CommandOp::Wait);
  REQUIRE(compiled.commands[1].frames == 9);
}

TEST_CASE("sequence edge from conditional_branch is a compile error", "[unit][event][graph]") {
  rat::EventGraph graph;
  rat::EventGraphNode branch;
  branch.id = "br";
  branch.kind = "conditional_branch";
  branch.branch_condition.type = rat::ConditionType::Switch;
  branch.branch_condition.id = 1;
  branch.branch_condition.bool_value = true;
  graph.nodes = {
      branch,
      rat::EventGraphNode{.id = "yes", .kind = "show_text", .text = "On"},
      rat::EventGraphNode{.id = "no", .kind = "show_text", .text = "Off"},
      rat::EventGraphNode{.id = "stray", .kind = "wait", .frames = 3},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "br"},
      rat::EventGraphEdge{.from = "br", .to = "yes", .branch = std::string("then")},
      rat::EventGraphEdge{.from = "br", .to = "no", .branch = std::string("else")},
      rat::EventGraphEdge{.from = "br", .to = "stray"},
      rat::EventGraphEdge{.from = "yes", .to = "exit"},
      rat::EventGraphEdge{.from = "no", .to = "exit"},
      rat::EventGraphEdge{.from = "stray", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE_FALSE(compiled.ok);
  REQUIRE(has_error_mentioning(compiled.issues, "sequence"));
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
  REQUIRE(again.map.events[0].pages[0].commands.empty());
}

TEST_CASE("dump omits commands when page has a graph", "[unit][event][graph][map]") {
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

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events[0].pages[0].commands.size() == 2);

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const nlohmann::json root = nlohmann::json::parse(serialized.json_text);
  const nlohmann::json& page = root.at("events").at(0).at("pages").at(0);
  REQUIRE(page.contains("graph"));
  REQUIRE_FALSE(page.contains("commands"));

  const rat::MapLoadResult again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events[0].pages[0].graph.has_value());
  REQUIRE(again.map.events[0].pages[0].graph->nodes.size() == 2);
  REQUIRE(again.map.events[0].pages[0].commands.empty());
}

TEST_CASE("dump still writes commands when page has no graph", "[unit][event][graph][map]") {
  rat::MapData map;
  map.schema_version = 1;
  map.id = "legacy";
  map.width = 2;
  map.height = 2;
  rat::EventDef event;
  event.id = "npc";
  event.tile = rat::TileCoord{0, 0};
  rat::EventPage page;
  page.trigger = rat::TriggerKind::Action;
  rat::Command text;
  text.op = rat::CommandOp::ShowText;
  text.text = "Legacy";
  page.commands.push_back(std::move(text));
  event.pages.push_back(std::move(page));
  map.events.push_back(std::move(event));

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(map);
  REQUIRE(serialized.ok);
  const nlohmann::json root = nlohmann::json::parse(serialized.json_text);
  const nlohmann::json& dumped = root.at("events").at(0).at("pages").at(0);
  REQUIRE(dumped.contains("commands"));
  REQUIRE_FALSE(dumped.contains("graph"));
  REQUIRE(dumped.at("commands").size() == 1);
  REQUIRE(dumped.at("commands").at(0).at("text") == "Legacy");
}

TEST_CASE("grey_yard dump and file pages are graph-only", "[unit][event][graph][map]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined for map file tests
#endif
  const std::string path = std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json";
  std::ifstream in(path, std::ios::binary);
  REQUIRE(in.good());
  const nlohmann::json file = nlohmann::json::parse(in);
  REQUIRE(file.at("id") == "grey_yard");
  for (const nlohmann::json& event : file.at("events")) {
    for (const nlohmann::json& page : event.at("pages")) {
      REQUIRE(page.contains("graph"));
      REQUIRE_FALSE(page.contains("commands"));
    }
  }

  const rat::MapLoadResult loaded = rat::load_map_from_file(path);
  REQUIRE(loaded.ok);
  REQUIRE_FALSE(loaded.map.events.empty());
  for (const rat::EventDef& event : loaded.map.events) {
    for (const rat::EventPage& page : event.pages) {
      REQUIRE(page.graph.has_value());
      REQUIRE(page.commands.empty());
    }
  }

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const nlohmann::json dumped = nlohmann::json::parse(serialized.json_text);
  for (const nlohmann::json& event : dumped.at("events")) {
    for (const nlohmann::json& page : event.at("pages")) {
      REQUIRE(page.contains("graph"));
      REQUIRE_FALSE(page.contains("commands"));
    }
  }
}

TEST_CASE("page without graph still loads commands", "[unit][event][graph][map]") {
  constexpr const char* kPage = R"({
    "trigger": "action",
    "commands": [ { "op": "show_text", "text": "No graph" } ]
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);
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
        "nodes": [ { "id": "n1", "kind": "not_a_kind", "params": { "id": "beep" } } ],
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

TEST_CASE("unknown graph kind is a compile error", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "n1", .kind = "not_a_kind"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "n1"},
      rat::EventGraphEdge{.from = "n1", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE_FALSE(compiled.ok);
  REQUIRE(has_error_mentioning(compiled.issues, "unknown"));
}

TEST_CASE("compile control_variable node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "var", .kind = "control_variable", .switch_id = 4, .int_value = 9},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "var"},
      rat::EventGraphEdge{.from = "var", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ControlVariable);
  REQUIRE(compiled.commands[0].id == 4);
  REQUIRE(compiled.commands[0].int_value == 9);
}

TEST_CASE("compile control_self_switch node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{
          .id = "ss", .kind = "control_self_switch", .bool_value = true, .self_switch = 'C'},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "ss"},
      rat::EventGraphEdge{.from = "ss", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ControlSelfSwitch);
  REQUIRE(compiled.commands[0].self_switch == 'C');
  REQUIRE(compiled.commands[0].bool_value == true);
}

TEST_CASE("compile transfer_player node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "tp",
                          .kind = "transfer_player",
                          .map_id = "grey_yard",
                          .x = 1.5f,
                          .y = 0.25f,
                          .z = 3.0f},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "tp"},
      rat::EventGraphEdge{.from = "tp", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::TransferPlayer);
  REQUIRE(compiled.commands[0].map_id == "grey_yard");
  REQUIRE(compiled.commands[0].x == 1.5f);
  REQUIRE(compiled.commands[0].y == 0.25f);
  REQUIRE(compiled.commands[0].z == 3.0f);
}

TEST_CASE("compile change_items node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "it",
                          .kind = "change_items",
                          .item_id = "door_key",
                          .item_delta = -1,
                          .key_item = true},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "it"},
      rat::EventGraphEdge{.from = "it", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::ChangeItems);
  REQUIRE(compiled.commands[0].item_id == "door_key");
  REQUIRE(compiled.commands[0].item_delta == -1);
  REQUIRE(compiled.commands[0].key_item == true);
}

TEST_CASE("compile play_se node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "se", .kind = "play_se", .text = "beep"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "se"},
      rat::EventGraphEdge{.from = "se", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::PlaySE);
  REQUIRE(compiled.commands[0].text == "beep");
}

TEST_CASE("compile set_move_route node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{
          .id = "mv",
          .kind = "set_move_route",
          .through = true,
          .route = {rat::RouteStep{.op = rat::RouteStepOp::Move, .dir = rat::RampDirection::East}},
      },
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "mv"},
      rat::EventGraphEdge{.from = "mv", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::SetMoveRoute);
  REQUIRE(compiled.commands[0].through == true);
  REQUIRE(compiled.commands[0].route.size() == 1);
  REQUIRE(compiled.commands[0].route[0].op == rat::RouteStepOp::Move);
  REQUIRE(compiled.commands[0].route[0].dir == rat::RampDirection::East);
}

TEST_CASE("compile comment node", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "c", .kind = "comment", .text = "note"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "c"},
      rat::EventGraphEdge{.from = "c", .to = "exit"},
  };

  const rat::EventGraphCompileResult compiled = rat::compile_event_graph(graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::Comment);
  REQUIRE(compiled.commands[0].text == "note");
}

TEST_CASE("play_se graph node round-trips through serialize/load", "[unit][event][graph][map]") {
  constexpr const char* kPage = R"({
    "trigger": "action",
    "commands": [ { "op": "play_se", "id": "beep" } ],
    "graph": {
      "nodes": [ { "id": "se", "kind": "play_se", "params": { "id": "beep" } } ],
      "edges": [ { "from": "entry", "to": "se" }, { "from": "se", "to": "exit" } ]
    }
  })";

  const std::string json = map_json_with_page(kPage);
  const rat::MapLoadResult loaded = rat::load_map_from_string(json);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events[0].pages[0].graph.has_value());
  REQUIRE(loaded.map.events[0].pages[0].graph->nodes.size() == 1);
  REQUIRE(loaded.map.events[0].pages[0].graph->nodes[0].kind == "play_se");
  REQUIRE(loaded.map.events[0].pages[0].graph->nodes[0].text == "beep");

  const rat::EventGraphCompileResult compiled =
      rat::compile_event_graph(*loaded.map.events[0].pages[0].graph);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.commands.size() == 1);
  REQUIRE(compiled.commands[0].op == rat::CommandOp::PlaySE);
  REQUIRE(compiled.commands[0].text == "beep");

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const rat::MapLoadResult again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events[0].pages[0].graph.has_value());
  REQUIRE(again.map.events[0].pages[0].graph->nodes[0].kind == "play_se");
  REQUIRE(again.map.events[0].pages[0].graph->nodes[0].text == "beep");
}

TEST_CASE("reverse-compile linear show_text then wait", "[unit][event][graph]") {
  rat::Command say;
  say.op = rat::CommandOp::ShowText;
  say.text = "Hello";
  rat::Command pause;
  pause.op = rat::CommandOp::Wait;
  pause.frames = 12;
  require_round_trip({say, pause});
}

TEST_CASE("reverse-compile control_switch and set_move_route", "[unit][event][graph]") {
  rat::Command sw;
  sw.op = rat::CommandOp::ControlSwitch;
  sw.id = 7;
  sw.bool_value = true;
  rat::Command move;
  move.op = rat::CommandOp::SetMoveRoute;
  move.through = true;
  move.route = {rat::RouteStep{.op = rat::RouteStepOp::Move, .dir = rat::RampDirection::East}};
  require_round_trip({sw, move});
}

TEST_CASE("reverse-compile nested conditional_branch then and else", "[unit][event][graph]") {
  rat::Command inner_then;
  inner_then.op = rat::CommandOp::ShowText;
  inner_then.text = "inner-then";
  rat::Command inner_else;
  inner_else.op = rat::CommandOp::ShowText;
  inner_else.text = "inner-else";
  rat::Command inner;
  inner.op = rat::CommandOp::ConditionalBranch;
  inner.branch_condition.type = rat::ConditionType::Switch;
  inner.branch_condition.id = 2;
  inner.branch_condition.bool_value = true;
  inner.then_commands = {inner_then};
  inner.else_commands = {inner_else};

  rat::Command outer_else;
  outer_else.op = rat::CommandOp::ShowText;
  outer_else.text = "outer-else";
  rat::Command outer;
  outer.op = rat::CommandOp::ConditionalBranch;
  outer.branch_condition.type = rat::ConditionType::Switch;
  outer.branch_condition.id = 1;
  outer.branch_condition.bool_value = true;
  outer.then_commands = {inner};
  outer.else_commands = {outer_else};

  rat::Command after;
  after.op = rat::CommandOp::Wait;
  after.frames = 5;
  require_round_trip({outer, after});
}

TEST_CASE("reverse-compile empty commands is entry to exit", "[unit][event][graph]") {
  const rat::EventGraph graph = rat::commands_to_graph({});
  REQUIRE(graph.nodes.empty());
  REQUIRE(graph.edges.size() == 1);
  REQUIRE(graph.edges[0].from == "entry");
  REQUIRE(graph.edges[0].to == "exit");
  require_round_trip({});
}

TEST_CASE("JSON commands without graph get reverse-compiled", "[unit][event][graph][map]") {
  constexpr const char* kPage = R"({
    "trigger": "action",
    "commands": [ { "op": "show_text", "text": "No graph" } ]
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);

  const rat::MapCompileResult compiled = rat::compile_map_data(loaded.map);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.runtime.data.events[0].pages[0].graph.has_value());
  REQUIRE_FALSE(compiled.runtime.data.events[0].pages[0].graph->nodes.empty());

  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  REQUIRE(runtime.map().events[0].pages[0].graph.has_value());
  REQUIRE_FALSE(runtime.map().events[0].pages[0].graph->nodes.empty());
}

TEST_CASE("EventRuntime uses graph not commands", "[unit][event][graph]") {
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
  REQUIRE(runtime.active_message() == "FromGraph");
}

TEST_CASE("graph_sequence_successor follows unlabeled edge", "[unit][event][graph]") {
  rat::EventGraph graph;
  graph.nodes = {
      rat::EventGraphNode{.id = "a", .kind = "show_text", .text = "A"},
      rat::EventGraphNode{.id = "b", .kind = "wait", .frames = 1},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "a"},
      rat::EventGraphEdge{.from = "a", .to = "b"},
      rat::EventGraphEdge{.from = "b", .to = "exit"},
  };

  REQUIRE(rat::graph_sequence_successor(graph, "entry") == "a");
  REQUIRE(rat::graph_sequence_successor(graph, "a") == "b");
  REQUIRE(rat::graph_sequence_successor(graph, "b") == "exit");
  REQUIRE_FALSE(rat::graph_sequence_successor(graph, "missing").has_value());
}

TEST_CASE("graph then and else targets from conditional_branch", "[unit][event][graph]") {
  rat::EventGraph graph;
  rat::EventGraphNode branch;
  branch.id = "br";
  branch.kind = "conditional_branch";
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

  REQUIRE(rat::graph_then_target(graph, "br") == "yes");
  REQUIRE(rat::graph_else_target(graph, "br") == "no");
  REQUIRE_FALSE(rat::graph_then_target(graph, "yes").has_value());
}

TEST_CASE("graph_else_target is empty when else-edge is omitted", "[unit][event][graph]") {
  rat::EventGraph graph;
  rat::EventGraphNode branch;
  branch.id = "br";
  branch.kind = "conditional_branch";
  graph.nodes = {
      branch,
      rat::EventGraphNode{.id = "yes", .kind = "show_text", .text = "On"},
  };
  graph.edges = {
      rat::EventGraphEdge{.from = "entry", .to = "br"},
      rat::EventGraphEdge{.from = "br", .to = "yes", .branch = std::string("then")},
      rat::EventGraphEdge{.from = "yes", .to = "exit"},
  };

  REQUIRE(rat::graph_then_target(graph, "br") == "yes");
  REQUIRE_FALSE(rat::graph_else_target(graph, "br").has_value());
}

TEST_CASE("command_from_node maps show_text payload", "[unit][event][graph]") {
  rat::EventGraphNode node;
  node.id = "say";
  node.kind = "show_text";
  node.text = "Hi";
  const rat::Command command = rat::command_from_node(node);
  REQUIRE(command.op == rat::CommandOp::ShowText);
  REQUIRE(command.text == "Hi");
}

TEST_CASE("EventRuntime follows graph then and else edges", "[unit][event][graph]") {
  constexpr const char* kPage = R"({
    "trigger": "autorun",
    "commands": [
      {
        "op": "conditional_branch",
        "condition": { "type": "switch", "id": 1, "value": true },
        "then": [ { "op": "show_text", "text": "CommandsThen" } ],
        "else": [ { "op": "show_text", "text": "CommandsElse" } ]
      }
    ],
    "graph": {
      "nodes": [
        { "id": "br", "kind": "conditional_branch",
          "params": { "condition": { "type": "switch", "id": 1, "value": true } } },
        { "id": "yes", "kind": "show_text", "params": { "text": "GraphThen" } },
        { "id": "no", "kind": "show_text", "params": { "text": "GraphElse" } }
      ],
      "edges": [
        { "from": "entry", "to": "br" },
        { "from": "br", "to": "yes", "branch": "then" },
        { "from": "br", "to": "no", "branch": "else" },
        { "from": "yes", "to": "exit" },
        { "from": "no", "to": "exit" }
      ]
    }
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);

  SECTION("then-edge when condition is true") {
    rat::GameState state;
    state.set_switch(1, true);
    rat::EventRuntime runtime;
    REQUIRE(runtime.load(loaded.map).ok);
    runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
    REQUIRE(runtime.active_message() == "GraphThen");
  }

  SECTION("else-edge when condition is false") {
    rat::GameState state;
    rat::EventRuntime runtime;
    REQUIRE(runtime.load(loaded.map).ok);
    runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
    REQUIRE(runtime.active_message() == "GraphElse");
  }
}

TEST_CASE("EventRuntime missing else-edge finishes like empty else_commands",
          "[unit][event][graph]") {
  constexpr const char* kPage = R"({
    "trigger": "autorun",
    "commands": [
      {
        "op": "conditional_branch",
        "condition": { "type": "switch", "id": 1, "value": true },
        "then": [ { "op": "show_text", "text": "CommandsThen" } ]
      },
      { "op": "control_switch", "id": 5, "value": true }
    ],
    "graph": {
      "nodes": [
        { "id": "br", "kind": "conditional_branch",
          "params": { "condition": { "type": "switch", "id": 1, "value": true } } },
        { "id": "yes", "kind": "show_text", "params": { "text": "GraphThen" } }
      ],
      "edges": [
        { "from": "entry", "to": "br" },
        { "from": "br", "to": "yes", "branch": "then" },
        { "from": "yes", "to": "exit" }
      ]
    }
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(map_json_with_page(kPage));
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE_FALSE(runtime.active_message().has_value());
  REQUIRE_FALSE(runtime.player_input_blocked());
  REQUIRE_FALSE(state.get_switch(5));
}

TEST_CASE("event graph node metrics grow with widget rows and place branch pins on option rows",
          "[unit][event][graph][edit]") {
  using Catch::Approx;
  const rat::EventGraphNodeMetrics wait = rat::event_graph_node_metrics("wait");
  const rat::EventGraphNodeMetrics show = rat::event_graph_node_metrics("show_text");
  const rat::EventGraphNodeMetrics branch = rat::event_graph_node_metrics("conditional_branch");
  const rat::EventGraphNodeMetrics route0 = rat::event_graph_node_metrics("set_move_route", 0);
  const rat::EventGraphNodeMetrics route3 = rat::event_graph_node_metrics("set_move_route", 3);
  const rat::EventGraphNodeMetrics entry = rat::event_graph_node_metrics("entry");
  const rat::EventGraphNodeMetrics transfer = rat::event_graph_node_metrics("transfer_player");

  REQUIRE(show.width == Approx(220.0f));
  REQUIRE(wait.width == Approx(220.0f));
  REQUIRE(show.height > wait.height);
  REQUIRE(transfer.height > wait.height);
  REQUIRE(entry.height < wait.height);
  REQUIRE(route3.height > route0.height);

  REQUIRE(wait.in_pin_y == Approx(wait.height * 0.5f));
  REQUIRE(wait.seq_out_pin_y == Approx(wait.height * 0.5f));
  REQUIRE(show.seq_out_pin_y == Approx(show.height * 0.5f));

  REQUIRE(branch.then_pin_y > branch.title_h);
  REQUIRE(branch.else_pin_y > branch.then_pin_y);
  REQUIRE(branch.else_pin_y < branch.height);
  REQUIRE(branch.then_pin_y != Approx(branch.height * 0.5f));
}
