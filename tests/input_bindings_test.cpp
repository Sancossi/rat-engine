#include <rat/input.hpp>
#include <rat/input_bindings.hpp>
#include <rat/map_data.hpp>
#include <rat/player.hpp>
#include <rat/simulation_session.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>

#if defined(__glfw3_h__) || defined(GLFW_TRUE)
#error input binding tests must stay headless (no GLFW)
#endif

using Catch::Approx;

namespace {

[[nodiscard]] rat::KeyboardState keys_down(std::initializer_list<const char*> names, bool ctrl = false,
                                           bool shift = false) {
  rat::KeyboardState state;
  for (const char* name : names) {
    state.keys_down.emplace_back(name);
  }
  state.ctrl = ctrl;
  state.shift = shift;
  return state;
}

[[nodiscard]] rat::KeyboardChord key(const char* name) {
  return rat::KeyboardChord{name, std::nullopt, std::nullopt};
}

[[nodiscard]] rat::MapData make_flat_map() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "bindings_flat";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);
  return map;
}

}  // namespace

TEST_CASE("default keyboard bindings map WASD to the same InputFrame as InputButtons",
          "[unit][input][bindings]") {
  const rat::InputBindings bindings = rat::default_input_bindings();
  const rat::KeyboardState keyboard = keys_down({"W", "D"});
  const rat::InputButtons buttons = rat::map_keyboard_buttons(keyboard, bindings);

  rat::InputButtons expected;
  expected.move_up = true;
  expected.move_right = true;
  CHECK(buttons.move_up == expected.move_up);
  CHECK(buttons.move_right == expected.move_right);
  CHECK_FALSE(buttons.move_down);
  CHECK_FALSE(buttons.move_left);

  const rat::InputFrame frame = rat::map_input_frame(buttons, {}, {});
  const rat::MoveInput expected_move = rat::world_aligned_move(1.0f, 1.0f);
  CHECK(frame.move.axis_x == expected_move.axis_x);
  CHECK(frame.move.axis_z == expected_move.axis_z);
}

TEST_CASE("rebound keys produce the same InputFrame and ignore the old WASD keys",
          "[unit][input][bindings]") {
  rat::InputBindings bindings = rat::default_input_bindings();
  bindings.move_up.keys = {key("I")};
  bindings.move_down.keys = {key("K")};
  bindings.move_left.keys = {key("J")};
  bindings.move_right.keys = {key("L")};
  bindings.jump.keys = {key("Period")};
  bindings.interact.keys = {key("U")};

  const rat::InputButtons rebound =
      rat::map_keyboard_buttons(keys_down({"I", "L", "Period", "U"}), bindings);
  CHECK(rebound.move_up);
  CHECK(rebound.move_right);
  CHECK(rebound.jump);
  CHECK(rebound.interact);

  const rat::InputButtons wasd =
      rat::map_keyboard_buttons(keys_down({"W", "D", "Space", "E"}), bindings);
  CHECK_FALSE(wasd.move_up);
  CHECK_FALSE(wasd.move_right);
  CHECK_FALSE(wasd.jump);
  CHECK_FALSE(wasd.interact);

  rat::InputButtons classic;
  classic.move_up = true;
  classic.move_right = true;
  classic.jump = true;
  classic.interact = true;
  const rat::InputFrame rebound_frame = rat::map_input_frame(rebound, {}, {});
  const rat::InputFrame classic_frame = rat::map_input_frame(classic, {}, {});
  CHECK(rebound_frame.move.axis_x == classic_frame.move.axis_x);
  CHECK(rebound_frame.move.axis_z == classic_frame.move.axis_z);
  CHECK(rebound_frame.jump_pressed == classic_frame.jump_pressed);
  CHECK(rebound_frame.interact_pressed == classic_frame.interact_pressed);
}

TEST_CASE("gamepad adapter maps dpad and face buttons to the same InputFrame",
          "[unit][input][bindings]") {
  rat::GamepadState pad;
  pad.connected = true;
  pad.buttons[rat::kGamepadButtonA] = true;
  pad.buttons[rat::kGamepadButtonX] = true;
  pad.buttons[rat::kGamepadButtonDpadUp] = true;
  pad.buttons[rat::kGamepadButtonDpadRight] = true;

  const rat::InputButtons buttons =
      rat::map_gamepad_buttons(pad, rat::default_input_bindings());
  CHECK(buttons.move_up);
  CHECK(buttons.move_right);
  CHECK(buttons.jump);
  CHECK(buttons.interact);

  rat::InputButtons classic;
  classic.move_up = true;
  classic.move_right = true;
  classic.jump = true;
  classic.interact = true;
  const rat::InputFrame pad_frame = rat::map_input_frame(buttons, {}, {});
  const rat::InputFrame classic_frame = rat::map_input_frame(classic, {}, {});
  CHECK(pad_frame.move.axis_x == classic_frame.move.axis_x);
  CHECK(pad_frame.move.axis_z == classic_frame.move.axis_z);
  CHECK(pad_frame.jump_pressed == classic_frame.jump_pressed);
  CHECK(pad_frame.interact_pressed == classic_frame.interact_pressed);
}

TEST_CASE("input bindings JSON round-trips a rebound layout", "[unit][input][bindings]") {
  rat::InputBindings original = rat::default_input_bindings();
  original.move_up.keys = {key("I")};
  original.jump.gamepad_buttons = {3};

  const std::string text = rat::write_input_bindings_json(original);
  const std::optional<rat::InputBindings> loaded = rat::read_input_bindings_json(text);
  REQUIRE(loaded.has_value());

  CHECK(rat::map_keyboard_buttons(keys_down({"I"}), *loaded).move_up);
  CHECK_FALSE(rat::map_keyboard_buttons(keys_down({"W"}), *loaded).move_up);

  rat::GamepadState pad;
  pad.connected = true;
  pad.buttons[3] = true;
  CHECK(rat::map_gamepad_buttons(pad, *loaded).jump);
  pad.buttons[3] = false;
  pad.buttons[rat::kGamepadButtonA] = true;
  CHECK_FALSE(rat::map_gamepad_buttons(pad, *loaded).jump);
}

TEST_CASE("gamepad stick axes map to move and a disconnected pad is ignored",
          "[unit][input][bindings]") {
  const rat::InputBindings bindings = rat::default_input_bindings();
  rat::GamepadState stick;
  stick.connected = true;
  stick.axes[rat::kGamepadAxisLeftX] = 1.0f;
  stick.axes[rat::kGamepadAxisLeftY] = -1.0f;
  const rat::InputButtons moved = rat::map_gamepad_buttons(stick, bindings);
  CHECK(moved.move_up);
  CHECK(moved.move_right);

  rat::GamepadState dead;
  dead.buttons[rat::kGamepadButtonA] = true;
  dead.axes[rat::kGamepadAxisLeftY] = -1.0f;
  const rat::InputButtons ignored = rat::map_gamepad_buttons(dead, bindings);
  CHECK_FALSE(ignored.jump);
  CHECK_FALSE(ignored.move_up);
}

TEST_CASE("keyboard and gamepad adapters merge into one InputButtons frame",
          "[unit][input][bindings]") {
  const rat::InputBindings bindings = rat::default_input_bindings();
  rat::GamepadState pad;
  pad.connected = true;
  pad.buttons[rat::kGamepadButtonA] = true;
  const rat::InputButtons buttons = rat::merge_input_buttons(
      rat::map_keyboard_buttons(keys_down({"W"}), bindings),
      rat::map_gamepad_buttons(pad, bindings));
  CHECK(buttons.move_up);
  CHECK(buttons.jump);

  const rat::InputFrame frame = rat::map_input_frame(buttons, {}, {});
  CHECK(frame.jump_pressed);
  CHECK(frame.jump_held);
}

TEST_CASE("invalid bindings JSON is rejected", "[unit][input][bindings]") {
  CHECK_FALSE(rat::read_input_bindings_json("not json").has_value());
  CHECK_FALSE(rat::read_input_bindings_json("{\"schema_version\":1}").has_value());
}

TEST_CASE("rebound keyboard InputFrame is what SimulationSession ticks",
          "[unit][input][bindings][sim]") {
  rat::InputBindings bindings = rat::default_input_bindings();
  bindings.move_up.keys = {key("I")};

  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  rat::PlayerBody start;
  start.x = 1.5f;
  start.y = 0.0f;
  start.z = 1.5f;
  start.speed = 5.0f;
  session.set_player(start);

  const rat::InputButtons down = rat::map_keyboard_buttons(keys_down({"I"}), bindings);
  const rat::InputFrame frame = rat::map_input_frame(down, {}, {});
  CHECK(frame.move.axis_z != 0.0f);
  session.tick(frame);
  CHECK(session.player().z < 1.5f);
  CHECK(session.player().z == Approx(session.state().player_z()).margin(1e-5f));
}
