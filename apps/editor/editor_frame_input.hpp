#pragma once

#include <rat/input_bindings.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace rat {

// One complete input sample feeds both ImGui and gameplay/viewport bindings.
// Key codes are the platform-independent GLFW key numbers; no GLFW types leak here.
struct EditorInputEvent {
  enum class Kind { Key, Character, MouseButton, Wheel, Focus, Cursor };
  Kind kind = Kind::Key;
  int code = 0;
  bool down = false;
  float x = 0.0f, y = 0.0f;
};

struct EditorFrameInput {
  std::vector<EditorInputEvent> events;
  std::array<bool, 512> keys{};
  std::array<bool, 5> mouse_buttons{};
  std::vector<std::uint32_t> characters;
  GamepadState gamepad;
  double cursor_x = 0.0;
  double cursor_y = 0.0;
  float wheel_x = 0.0f;
  float wheel_y = 0.0f;
  int logical_width = 1280;
  int logical_height = 720;
  int framebuffer_width = 1280;
  int framebuffer_height = 720;
  bool focused = true;
  bool close_requested = false;

  [[nodiscard]] InputButtons buttons(const InputBindings& bindings) const;
};

}  // namespace rat
