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

  std::filesystem::remove(path, ec);
}
