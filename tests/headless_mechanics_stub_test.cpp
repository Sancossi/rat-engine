#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_test_macros.hpp>

// Headless mechanics: drive a tiny JSON scenario without GLFW/bgfx.
TEST_CASE("Headless mechanics: grey_yard autorun + inventory change path", "[mechanics]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded = rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_message().has_value());
  runtime.acknowledge_message();

  // Drain autorun (text + control_variable + wait).
  for (int i = 0; i < 5; ++i) {
    runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
    if (runtime.active_message().has_value()) {
      runtime.acknowledge_message();
    }
  }
  REQUIRE(state.get_variable(0) == 1);

  // Step onto welcome mat before reading the sign (switch 1 still false).
  rat::PlayerBody player;
  player.x = 0.0f;
  player.z = 0.0f;
  runtime.update(state, player, false, 1.0f / 60.0f);
  if (runtime.active_message().has_value()) {
    REQUIRE(runtime.active_message() == "Welcome to the grey yard.");
    runtime.acknowledge_message();
    runtime.update(state, player, false, 1.0f / 60.0f);
  }
  REQUIRE(state.has_item("rusty_cog"));
}
