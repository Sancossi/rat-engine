#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

void drain_messages(rat::EventRuntime& runtime, rat::GameState& state, rat::PlayerBody& player,
                    int max_steps = 30) {
  for (int i = 0; i < max_steps; ++i) {
    runtime.update(state, player, false, 1.0f / 60.0f);
    if (runtime.active_message().has_value()) {
      runtime.acknowledge_message();
      continue;
    }
    if (!runtime.player_input_blocked()) {
      return;
    }
  }
}

void interact_at(rat::EventRuntime& runtime, rat::GameState& state, rat::PlayerBody& player) {
  runtime.update(state, player, true, 1.0f / 60.0f);
  drain_messages(runtime, state, player);
}

}  // namespace

TEST_CASE("Headless mechanics: grey_yard cog quest end-to-end", "[mechanics][quest]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events.size() >= 3);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.z = 0.5f;

  drain_messages(runtime, state, player);
  REQUIRE(state.get_variable(0) == 1);

  // Accept quest from foreman at the center of tile (-2, 2).
  player.x = -1.5f;
  player.z = 2.5f;
  interact_at(runtime, state, player);
  REQUIRE(state.get_switch(1));
  REQUIRE_FALSE(state.has_item("rusty_cog"));

  // Loot scrap east of crates at the center of tile (6, 0).
  player.x = 6.5f;
  player.z = 0.5f;
  interact_at(runtime, state, player);
  REQUIRE(state.has_item("rusty_cog"));
  REQUIRE(state.get_self_switch("scrap_pile", 'A'));
  REQUIRE_FALSE(state.get_switch(3));

  // Turn in at foreman.
  player.x = -1.5f;
  player.z = 2.5f;
  interact_at(runtime, state, player);
  REQUIRE(state.get_switch(2));
  REQUIRE_FALSE(state.has_item("rusty_cog"));

  // Completion page still talks.
  interact_at(runtime, state, player);
  REQUIRE(state.get_switch(2));
}

TEST_CASE("Headless mechanics: grey_yard apprentice talks without starting the cog quest",
          "[mechanics][quest]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.z = 0.5f;

  drain_messages(runtime, state, player);
  REQUIRE(state.get_variable(0) == 1);
  REQUIRE_FALSE(state.get_switch(1));

  // Ground apprentice at the center of tile (-3, 1), off spawn / foreman / scrap / crates / loft.
  player.x = -2.5f;
  player.z = 1.5f;
  REQUIRE(runtime.has_action_prompt(player, state));

  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(runtime.active_message().has_value());
  // UTF-8 «Ученик» (not a source-charset literal) so MSVC without /utf-8 cannot mangle it.
  const std::string uchenik{"\xD0\xA3\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xBA"};
  CHECK(runtime.active_message()->find(uchenik) != std::string::npos);
  REQUIRE_FALSE(state.get_switch(1));
  REQUIRE_FALSE(state.get_switch(2));
  REQUIRE_FALSE(state.has_item("rusty_cog"));

  drain_messages(runtime, state, player);
  REQUIRE_FALSE(state.get_switch(1));
}

TEST_CASE("Headless mechanics: grey_yard walker leaves spawn on set_move_route",
          "[mechanics][quest][route]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  REQUIRE_FALSE(loaded.map.events.empty());
  REQUIRE(loaded.map.events.back().id == "loft_plank");
  REQUIRE(loaded.map.events.back().y.has_value());
  REQUIRE(*loaded.map.events.back().y == 2.0f);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.z = 0.5f;

  drain_messages(runtime, state, player);
  REQUIRE(state.get_variable(0) == 1);
  REQUIRE_FALSE(state.get_switch(1));
  REQUIRE_FALSE(state.get_switch(2));
  REQUIRE_FALSE(state.has_item("rusty_cog"));

  runtime.update(state, player, false, 1.0f / 60.0f);
  runtime.update(state, player, false, 1.0f / 60.0f);
  const auto mid = runtime.event_overlay("yard_walker");
  REQUIRE(mid.has_value());
  REQUIRE(mid->tile.x == -4);
  REQUIRE(mid->tile.z == -1);
  REQUIRE(mid->x > -3.5f);

  bool left_spawn = false;
  for (int i = 0; i < 30; ++i) {
    runtime.update(state, player, false, 1.0f / 60.0f);
    const auto overlay = runtime.event_overlay("yard_walker");
    if (overlay.has_value() && (overlay->tile.x != -4 || overlay->tile.z != -1)) {
      left_spawn = true;
      break;
    }
  }
  REQUIRE(left_spawn);
  REQUIRE_FALSE(state.get_switch(1));
  REQUIRE_FALSE(state.get_switch(2));
  REQUIRE_FALSE(state.has_item("rusty_cog"));
}
