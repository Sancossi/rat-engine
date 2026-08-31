#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("Event runtime autorun runs once and blocks until finished", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "intro",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "conditions": [ { "type": "variable", "id": 0, "op": "==", "value": 0 } ],
            "commands": [
              { "op": "show_text", "text": "Hello" },
              { "op": "control_variable", "id": 0, "value": 1 }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(runtime.player_input_blocked());
  REQUIRE(runtime.active_message() == "Hello");
  REQUIRE(state.get_variable(0) == 0);

  runtime.acknowledge_message();
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(state.get_variable(0) == 1);
  REQUIRE_FALSE(runtime.player_input_blocked());
  REQUIRE_FALSE(runtime.active_message().has_value());
}

TEST_CASE("Event runtime action trigger requires interact near tile", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "sign",
        "tile": { "x": 2, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [ { "op": "control_switch", "id": 5, "value": true } ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody far;
  far.x = 0.0f;
  far.z = 0.0f;
  REQUIRE_FALSE(runtime.has_action_prompt(far, state));
  runtime.update(state, far, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(5));

  rat::PlayerBody near;
  near.x = 2.0f;
  near.z = 2.0f;
  REQUIRE(runtime.has_action_prompt(near, state));
  runtime.update(state, near, true, 1.0f / 60.0f);
  REQUIRE(state.get_switch(5));
}

TEST_CASE("Event runtime enforces Parallel limits", "[unit][events]") {
  // 9 parallel events each spinning wait(0)+comment — only 8 may be active;
  // command budget per frame is 32.
  std::string json = R"({"schema_version":1,"id":"p","width":8,"height":8,"events":[)";
  for (int i = 0; i < 9; ++i) {
    if (i > 0) {
      json += ',';
    }
    json += R"({"id":"p)" + std::to_string(i) + R"(","tile":{"x":)" + std::to_string(i) +
            R"(,"z":0},"pages":[{"trigger":"parallel","commands":[)"
            R"({"op":"comment","text":"tick"},{"op":"wait","frames":0}]}]})" ;
  }
  json += "]}";

  const auto loaded = rat::load_map_from_string(json);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(runtime.active_parallel_count() <= 8);
  REQUIRE(runtime.last_parallel_commands_executed() <= 32);
}
