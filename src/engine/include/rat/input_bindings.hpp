#pragma once

#include "rat/input.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

// Indices match GLFW's standard gamepad layout so NativeWindow can copy
// glfwGetGamepadState() arrays without translating buttons.
inline constexpr int kGamepadButtonCount = 15;
inline constexpr int kGamepadAxisCount = 6;
inline constexpr int kGamepadButtonA = 0;
inline constexpr int kGamepadButtonX = 2;
inline constexpr int kGamepadButtonDpadUp = 11;
inline constexpr int kGamepadButtonDpadRight = 12;
inline constexpr int kGamepadButtonDpadDown = 13;
inline constexpr int kGamepadButtonDpadLeft = 14;
inline constexpr int kGamepadAxisLeftX = 0;
inline constexpr int kGamepadAxisLeftY = 1;
inline constexpr float kGamepadAxisThreshold = 0.5f;

struct KeyboardChord {
  std::string key;
  std::optional<bool> ctrl;
  std::optional<bool> shift;
};

struct GamepadAxisBinding {
  int axis = -1;
  bool negative = false;
  float threshold = kGamepadAxisThreshold;
};

struct ActionSources {
  std::vector<KeyboardChord> keys;
  std::vector<int> gamepad_buttons;
  std::vector<GamepadAxisBinding> gamepad_axes;
};

struct KeyboardState {
  std::vector<std::string> keys_down;
  bool ctrl = false;
  bool shift = false;
};

struct GamepadState {
  bool connected = false;
  std::array<bool, kGamepadButtonCount> buttons{};
  std::array<float, kGamepadAxisCount> axes{};
};

struct InputBindings {
  ActionSources move_up;
  ActionSources move_down;
  ActionSources move_left;
  ActionSources move_right;
  ActionSources jump;
  ActionSources interact;
  ActionSources toggle_mode;
  ActionSources hot_apply;
  ActionSources cycle_camera;
  ActionSources debug_snapshot;
  ActionSources undo;
  ActionSources redo;
};

[[nodiscard]] InputBindings default_input_bindings();

[[nodiscard]] InputButtons map_keyboard_buttons(const KeyboardState& keyboard,
                                                const InputBindings& bindings);
[[nodiscard]] InputButtons map_gamepad_buttons(const GamepadState& gamepad,
                                               const InputBindings& bindings);
[[nodiscard]] InputButtons merge_input_buttons(const InputButtons& a, const InputButtons& b);

[[nodiscard]] std::string write_input_bindings_json(const InputBindings& bindings);
[[nodiscard]] std::optional<InputBindings> read_input_bindings_json(std::string_view text);

}  // namespace rat
