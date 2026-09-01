#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_approx.hpp>
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

  // This overlaps the old whole-cell AABB, but is still visibly too far from marker center.
  rat::PlayerBody early;
  early.x = 1.7f;
  early.z = 2.5f;
  REQUIRE_FALSE(runtime.has_action_prompt(early, state));
  runtime.update(state, early, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(5));

  rat::PlayerBody near;
  near.x = 2.1f;
  near.z = 2.1f;
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

TEST_CASE("Event runtime control_self_switch flips page conditions", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "chest",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "conditions": [],
            "commands": [
              { "op": "show_text", "text": "Loot" },
              { "op": "control_self_switch", "key": "A", "value": true }
            ]
          },
          {
            "trigger": "action",
            "conditions": [ { "type": "self_switch", "key": "A", "value": true } ],
            "commands": [ { "op": "show_text", "text": "Empty" } ]
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

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;

  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Loot");
  runtime.acknowledge_message();
  runtime.update(state, player, false, 1.0f / 60.0f);
  REQUIRE(state.get_self_switch("chest", 'A'));

  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Empty");
}

TEST_CASE("Event runtime action and touch require matching height", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "height_gate",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [
        0, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 3, 0,
        0, 0, 0, 0
      ]
    },
    "events": [
      {
        "id": "upper_action",
        "tile": { "x": 2, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 11, "value": true }]
          }
        ]
      },
      {
        "id": "upper_touch",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "player_touch",
            "commands": [{ "op": "control_switch", "id": 12, "value": true }]
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

  rat::PlayerBody player;
  player.x = 2.5f;
  player.z = 2.5f;
  player.y = 0.0f;
  REQUIRE_FALSE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(11));

  player.y = 3.0f;
  REQUIRE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(state.get_switch(11));

  rat::PlayerBody touch_player;
  touch_player.x = 1.5f;
  touch_player.z = 1.5f;
  touch_player.y = 0.0f;
  runtime.update(state, touch_player, false, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(12));

  touch_player.y = 2.0f;
  runtime.update(state, touch_player, false, 1.0f / 60.0f);
  REQUIRE(state.get_switch(12));
}

TEST_CASE("Event runtime transfer player resamples target height", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "transfer_height",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 7, 0,
        0, 0, 0, 0
      ]
    },
    "events": [
      {
        "id": "warp",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "action",
            "commands": [
              { "op": "transfer_player", "map_id": "transfer_height", "x": 2.5, "y": 123.0, "z": 2.5 }
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

  rat::PlayerBody player;
  player.x = 0.5f;
  player.z = 0.5f;
  player.y = 0.0f;

  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(state.map_id() == "transfer_height");
  REQUIRE(state.player_x() == Catch::Approx(2.5f));
  REQUIRE(state.player_z() == Catch::Approx(2.5f));
  REQUIRE(state.player_y() == Catch::Approx(7.0f));
}

TEST_CASE("Event runtime allows action along same ramp surface", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "ramp_action",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
      ]
    },
    "ramps": [
      { "tile": { "x": 1, "z": 1 }, "direction": "east", "low_y": 1.0, "high_y": 2.0 }
    ],
    "events": [
      {
        "id": "ramp_switch",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 21, "value": true }]
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

  rat::PlayerBody player;
  player.x = 1.05f;
  player.z = 1.5f;
  player.y = 1.05f;
  REQUIRE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(state.get_switch(21));
}

TEST_CASE("Event runtime allows jumping above same flat event ground", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "flat_jump_ok",
    "width": 3,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 3,
      "height": 3,
      "ground_y": [
        0, 0, 0,
        0, 3, 0,
        0, 0, 0
      ]
    },
    "events": [
      {
        "id": "jump_sign",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 22, "value": true }]
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

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;
  player.y = 4.2f;  // airborne above same local ground.
  REQUIRE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(state.get_switch(22));
}

TEST_CASE("Event runtime rejects player body below local ground", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "below_ground",
    "width": 3,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 3,
      "height": 3,
      "ground_y": [
        0, 0, 0,
        0, 2, 0,
        0, 0, 0
      ]
    },
    "events": [
      {
        "id": "bad_pose",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 23, "value": true }]
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

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;
  player.y = 1.7f;
  REQUIRE_FALSE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(23));
}

TEST_CASE("Event runtime rejects adjacent lower flat to upper event for action and volume",
          "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "flat_delta_gate",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [
        0, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
      ]
    },
    "events": [
      {
        "id": "upper_tile",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 24, "value": true }]
          }
        ]
      },
      {
        "id": "upper_volume",
        "volume": { "min_x": 1.0, "min_z": 1.0, "max_x": 2.0, "max_z": 2.0 },
        "pages": [
          {
            "trigger": "player_touch",
            "commands": [{ "op": "control_switch", "id": 25, "value": true }]
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

  rat::PlayerBody lower;
  lower.x = 1.5f;
  lower.z = 0.95f;
  lower.y = 0.0f;
  REQUIRE_FALSE(runtime.has_action_prompt(lower, state));
  runtime.update(state, lower, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(24));

  runtime.update(state, lower, false, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(25));
}

TEST_CASE("Event runtime same ramp rise bypasses height delta", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "same_ramp",
    "width": 3,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 3,
      "height": 3,
      "ground_y": [
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
      ]
    },
    "ramps": [
      { "tile": { "x": 1, "z": 1 }, "direction": "east", "low_y": 1.0, "high_y": 5.0 }
    ],
    "events": [
      {
        "id": "ramp_event",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          { "trigger": "action", "commands": [{ "op": "control_switch", "id": 31, "value": true }] }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 1.0f;
  player.z = 1.5f;
  player.y = 1.0f;  // event center is y=3, delta=2 on same ramp.
  REQUIRE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(state.get_switch(31));
}

TEST_CASE("Event runtime lower flat near ramp event is rejected", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "flat_vs_ramp",
    "width": 3,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 3,
      "height": 3,
      "ground_y": [
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
      ]
    },
    "ramps": [
      { "tile": { "x": 1, "z": 1 }, "direction": "east", "low_y": 1.0, "high_y": 5.0 }
    ],
    "events": [
      {
        "id": "ramp_event",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          { "trigger": "action", "commands": [{ "op": "control_switch", "id": 32, "value": true }] }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 0.9f;  // flat tile, still within action radius.
  player.z = 1.5f;
  player.y = 0.0f;
  REQUIRE_FALSE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(32));
}

TEST_CASE("Event runtime ramp player near elevated flat event is rejected", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "ramp_vs_flat",
    "width": 5,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 5,
      "height": 3,
      "ground_y": [
        0, 0, 0, 0, 0,
        0, 0, 0, 3, 0,
        0, 0, 0, 0, 0
      ]
    },
    "ramps": [
      { "tile": { "x": 2, "z": 1 }, "direction": "east", "low_y": 0.0, "high_y": 1.0 }
    ],
    "events": [
      {
        "id": "high_flat",
        "tile": { "x": 3, "z": 1 },
        "pages": [
          { "trigger": "action", "commands": [{ "op": "control_switch", "id": 33, "value": true }] }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 2.95f;
  player.z = 1.5f;
  player.y = 0.95f;
  REQUIRE_FALSE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(33));
}

TEST_CASE("Event runtime different adjacent ramps do not bypass delta", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "adjacent_ramps",
    "width": 4,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 3,
      "ground_y": [
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
      ]
    },
    "ramps": [
      { "tile": { "x": 1, "z": 1 }, "direction": "east", "low_y": 0.0, "high_y": 4.0 },
      { "tile": { "x": 2, "z": 1 }, "direction": "east", "low_y": 0.0, "high_y": 4.0 }
    ],
    "events": [
      {
        "id": "ramp2_event",
        "tile": { "x": 2, "z": 1 },
        "pages": [
          { "trigger": "action", "commands": [{ "op": "control_switch", "id": 34, "value": true }] }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 1.95f;
  player.z = 1.5f;
  player.y = 3.8f;
  REQUIRE_FALSE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state.get_switch(34));
}

TEST_CASE("Event runtime elevation mutation rebuilds surface query safely", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "runtime_mutation",
    "width": 3,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 3,
      "height": 3,
      "ground_y": [
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
      ]
    },
    "events": [
      {
        "id": "talk",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "show_text", "text": "Hi" }]
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

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;
  player.y = 0.0f;
  REQUIRE(runtime.has_action_prompt(player, state));

  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Hi");
  REQUIRE(runtime.player_input_blocked());

  const auto elevate = runtime.set_tile_elevation(1, 1, 2.0f);
  REQUIRE(elevate.ok);
  REQUIRE(runtime.active_message() == "Hi");
  REQUIRE(runtime.player_input_blocked());

  runtime.acknowledge_message();
  runtime.update(state, player, false, 1.0f / 60.0f);
  REQUIRE_FALSE(runtime.has_action_prompt(player, state));

  player.y = 2.0f;
  REQUIRE(runtime.has_action_prompt(player, state));
}

TEST_CASE("Event runtime failed elevation mutation is transactional for legacy map",
          "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "legacy_txn",
    "width": 3,
    "height": 3,
    "events": [
      {
        "id": "talk",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "show_text", "text": "Legacy" }]
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
  const rat::MapData before = runtime.map();

  rat::PlayerBody player;
  player.x = 1.5f;
  player.y = 0.0f;
  player.z = 1.5f;
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Legacy");
  REQUIRE(runtime.player_input_blocked());

  const auto failed_oor = runtime.set_tile_elevation(99, 99, 5.0f);
  REQUIRE_FALSE(failed_oor.ok);
  REQUIRE(runtime.map().schema_version == before.schema_version);
  REQUIRE(runtime.map().height_grid.origin_x == before.height_grid.origin_x);
  REQUIRE(runtime.map().height_grid.origin_z == before.height_grid.origin_z);
  REQUIRE(runtime.map().height_grid.width == before.height_grid.width);
  REQUIRE(runtime.map().height_grid.height == before.height_grid.height);
  REQUIRE(runtime.map().height_grid.ground_y == before.height_grid.ground_y);
  REQUIRE(runtime.map().ramps.empty());
  REQUIRE(runtime.active_message() == "Legacy");
  REQUIRE(runtime.player_input_blocked());

  rat::RampDef invalid_ramp;
  invalid_ramp.tile = rat::TileCoord{1, 1};
  invalid_ramp.direction = static_cast<rat::RampDirection>(999);
  invalid_ramp.low_y = 0.0f;
  invalid_ramp.high_y = 1.0f;

  const auto failed_ramp = runtime.upsert_ramp_elevation(invalid_ramp);
  REQUIRE_FALSE(failed_ramp.ok);
  REQUIRE(runtime.map().schema_version == before.schema_version);
  REQUIRE(runtime.map().height_grid.ground_y == before.height_grid.ground_y);
  REQUIRE(runtime.map().ramps.empty());
  REQUIRE(runtime.active_message() == "Legacy");
  REQUIRE(runtime.player_input_blocked());

  runtime.acknowledge_message();
  runtime.update(state, player, false, 1.0f / 60.0f);
  REQUIRE_FALSE(runtime.active_message().has_value());
}

TEST_CASE("Event runtime rejects set/adjust on ramp tile transactionally", "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "runtime_ramp_guard",
    "width": 3,
    "height": 3,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 3,
      "height": 3,
      "ground_y": [
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
      ]
    },
    "ramps": [
      { "tile": { "x": 1, "z": 1 }, "direction": "east", "low_y": 1.0, "high_y": 2.0 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  const auto before_grid = runtime.map().height_grid.ground_y;
  const std::size_t before_ramp_count = runtime.map().ramps.size();
  REQUIRE(before_ramp_count == 1);
  const auto before_ramp = runtime.map().ramps[0];

  const auto set_result = runtime.set_tile_elevation(1, 1, 9.0f);
  REQUIRE_FALSE(set_result.ok);
  REQUIRE_FALSE(set_result.error.empty());
  REQUIRE(runtime.map().height_grid.ground_y == before_grid);
  REQUIRE(runtime.map().ramps.size() == before_ramp_count);
  REQUIRE(runtime.map().ramps[0].tile.x == before_ramp.tile.x);
  REQUIRE(runtime.map().ramps[0].tile.z == before_ramp.tile.z);
  REQUIRE(runtime.map().ramps[0].direction == before_ramp.direction);
  REQUIRE(runtime.map().ramps[0].low_y == Catch::Approx(before_ramp.low_y));
  REQUIRE(runtime.map().ramps[0].high_y == Catch::Approx(before_ramp.high_y));

  const auto adjust_result = runtime.adjust_tile_elevation(1, 1, 1.0f);
  REQUIRE_FALSE(adjust_result.ok);
  REQUIRE_FALSE(adjust_result.error.empty());
  REQUIRE(runtime.map().height_grid.ground_y == before_grid);
  REQUIRE(runtime.map().ramps.size() == before_ramp_count);
  REQUIRE(runtime.map().ramps[0].tile.x == before_ramp.tile.x);
  REQUIRE(runtime.map().ramps[0].tile.z == before_ramp.tile.z);
  REQUIRE(runtime.map().ramps[0].direction == before_ramp.direction);
  REQUIRE(runtime.map().ramps[0].low_y == Catch::Approx(before_ramp.low_y));
  REQUIRE(runtime.map().ramps[0].high_y == Catch::Approx(before_ramp.high_y));
}

TEST_CASE("set_blockers keeps active message and autorun lock; set_events clears them",
          "[unit][events]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "blockers": [{"min_x": 0, "min_z": 0, "max_x": 1, "max_z": 1}],
    "events": [
      {
        "id": "intro",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [ { "op": "show_text", "text": "Hello" } ]
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
  REQUIRE(runtime.active_message() == "Hello");
  REQUIRE(runtime.player_input_blocked());

  auto blockers = runtime.map().blockers;
  REQUIRE(blockers.size() == 1);
  blockers[0].bounds.max_x = 2.0f;
  runtime.set_blockers(std::move(blockers));
  REQUIRE(runtime.active_message() == "Hello");
  REQUIRE(runtime.player_input_blocked());
  REQUIRE(runtime.map().blockers[0].bounds.max_x == Catch::Approx(2.0f));

  auto events = runtime.map().events;
  runtime.set_events(std::move(events));
  REQUIRE_FALSE(runtime.active_message().has_value());
  REQUIRE_FALSE(runtime.player_input_blocked());

  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Hello");
}

TEST_CASE("player_overlaps uses circle vs event AABB not square corner", "[event][collision]") {
  rat::MapData map;
  map.schema_version = 2;
  map.tile_size = 1.0f;
  map.width = 4;
  map.height = 4;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);

  rat::EventDef event;
  event.id = "vol";
  event.volume = rat::Aabb2{0.0f, 0.0f, 1.0f, 1.0f};
  map.events.push_back(event);

  rat::EventRuntime runtime;
  runtime.load(map);
  const rat::EventDef& vol = runtime.map().events[0];

  rat::PlayerBody corner;
  corner.x = 1.35f;
  corner.y = 0.0f;
  corner.z = 1.35f;
  corner.half_extent = 0.4f;
  REQUIRE_FALSE(runtime.player_overlaps(vol, corner));

  rat::PlayerBody axis;
  axis.x = 1.3f;
  axis.y = 0.0f;
  axis.z = 0.5f;
  axis.half_extent = 0.4f;
  REQUIRE(runtime.player_overlaps(vol, axis));
}
