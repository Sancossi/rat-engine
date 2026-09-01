#include <rat/app_mode.hpp>
#include <rat/debug_snapshot.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <system_error>

TEST_CASE("write_debug_snapshot round-trips a headless fixture", "[unit][debug]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "snap",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "intro",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
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
  state.set_switch(3, true);
  state.set_variable(2, 9);
  state.add_item("rusty_cog", 1, true);

  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  runtime.update(state, player, false, 1.0f / 60.0f);

  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.jump_offset = 1.25f;
  jump.grounded = false;

  const rat::DebugSnapshot written = rat::make_debug_snapshot(
      42, rat::AppMode::Play, player, jump, runtime, state);

  const auto path = std::filesystem::temp_directory_path() / "rat-debug-snapshot-test.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_debug_snapshot(path.string(), written));

  const auto read = rat::read_debug_snapshot(path.string());
  REQUIRE(read.has_value());
  CHECK(read->sim_frame == 42);
  CHECK(read->app_mode == "PLAY");
  CHECK(read->player_x == 0.5f);
  CHECK(read->player_y == 0.0f);
  CHECK(read->player_z == 0.5f);
  CHECK(read->jump.jump_offset == 1.25f);
  CHECK_FALSE(read->jump.grounded);
  REQUIRE_FALSE(read->overlapping_event_ids.empty());
  CHECK(read->overlapping_event_ids[0] == "intro");
  REQUIRE(read->active_interpreter.has_value());
  CHECK(read->active_interpreter->event_id == "intro");
  CHECK(read->active_interpreter->page_index == 0);
  CHECK(read->active_interpreter->waiting_message);
  CHECK(read->switches.at(3) == true);
  CHECK(read->variables.at(2) == 9);
  REQUIRE(read->items.size() == 1);
  CHECK(read->items[0].id == "rusty_cog");
  REQUIRE(read->active_message.has_value());
  CHECK(*read->active_message == "Hello");
  REQUIRE(read->event_why_not.size() == 1);
  CHECK(read->event_why_not[0].id == "intro");
  CHECK(read->event_why_not[0].reason == "already_running");
  CHECK(read->event_why_not_reason == "already_running");

  std::filesystem::remove(path, ec);
}

TEST_CASE("make_debug_snapshot primary why-not follows selected_event_id", "[unit][debug][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "snap_selected_why",
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
        0, 0, 3, 0,
        0, 0, 0, 0
      ]
    },
    "events": [
      {
        "id": "sign",
        "tile": { "x": 2, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 11, "value": true }]
          }
        ]
      },
      {
        "id": "gated",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "action",
            "conditions": [{ "type": "switch", "id": 1, "value": true }],
            "commands": [{ "op": "control_switch", "id": 2, "value": true }]
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
  player.y = 0.0f;
  player.z = 2.5f;

  const rat::DebugSnapshot fallback = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true);
  REQUIRE(fallback.event_why_not.size() == 2);
  CHECK(fallback.event_why_not[0].id == "sign");
  CHECK(fallback.event_why_not[0].reason == "height");
  CHECK(fallback.event_why_not[1].id == "gated");
  CHECK(fallback.event_why_not[1].reason == "conditions");
  CHECK(fallback.event_why_not_reason == "height");

  const rat::DebugSnapshot selected = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true, "gated");
  REQUIRE(selected.event_why_not.size() == 2);
  CHECK(selected.event_why_not[0].id == "sign");
  CHECK(selected.event_why_not[1].id == "gated");
  CHECK(selected.event_why_not_reason == "conditions");
  CHECK(selected.event_why_not_reason != fallback.event_why_not_reason);

  const rat::DebugSnapshot unknown = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true, "missing");
  CHECK(unknown.event_why_not_reason == fallback.event_why_not_reason);
  REQUIRE(unknown.event_why_not.size() == 2);
}
