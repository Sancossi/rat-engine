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
