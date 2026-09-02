#include "native_window.hpp"

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#ifdef None
#undef None
#endif

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <utility>

namespace rat {
namespace {

[[nodiscard]] int glfw_key_named(std::string_view name) {
  if (name.size() == 1) {
    const char c = name[0];
    if (c >= 'A' && c <= 'Z') {
      return GLFW_KEY_A + (c - 'A');
    }
    if (c >= '0' && c <= '9') {
      return GLFW_KEY_0 + (c - '0');
    }
  }
  if (name.size() >= 2 && name[0] == 'F') {
    int index = 0;
    for (std::size_t i = 1; i < name.size(); ++i) {
      if (name[i] < '0' || name[i] > '9') {
        index = -1;
        break;
      }
      index = index * 10 + (name[i] - '0');
    }
    if (index >= 1 && index <= 12) {
      return GLFW_KEY_F1 + (index - 1);
    }
  }
  if (name == "Space") {
    return GLFW_KEY_SPACE;
  }
  if (name == "Up") {
    return GLFW_KEY_UP;
  }
  if (name == "Down") {
    return GLFW_KEY_DOWN;
  }
  if (name == "Left") {
    return GLFW_KEY_LEFT;
  }
  if (name == "Right") {
    return GLFW_KEY_RIGHT;
  }
  if (name == "Enter") {
    return GLFW_KEY_ENTER;
  }
  if (name == "Tab") {
    return GLFW_KEY_TAB;
  }
  if (name == "Escape") {
    return GLFW_KEY_ESCAPE;
  }
  if (name == "Backspace") {
    return GLFW_KEY_BACKSPACE;
  }
  if (name == "Period") {
    return GLFW_KEY_PERIOD;
  }
  if (name == "Comma") {
    return GLFW_KEY_COMMA;
  }
  if (name == "Minus") {
    return GLFW_KEY_MINUS;
  }
  if (name == "Equal") {
    return GLFW_KEY_EQUAL;
  }
  if (name == "Slash") {
    return GLFW_KEY_SLASH;
  }
  if (name == "Semicolon") {
    return GLFW_KEY_SEMICOLON;
  }
  if (name == "Apostrophe") {
    return GLFW_KEY_APOSTROPHE;
  }
  return -1;
}

template <typename Fn>
void for_each_sources(const InputBindings& bindings, Fn&& fn) {
  fn(bindings.move_up);
  fn(bindings.move_down);
  fn(bindings.move_left);
  fn(bindings.move_right);
  fn(bindings.jump);
  fn(bindings.interact);
  fn(bindings.toggle_mode);
  fn(bindings.hot_apply);
  fn(bindings.cycle_camera);
  fn(bindings.debug_snapshot);
  fn(bindings.undo);
  fn(bindings.redo);
}

[[nodiscard]] KeyboardState sample_keyboard(GLFWwindow* window, const InputBindings& bindings) {
  KeyboardState state;
  const auto down = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
  state.ctrl = down(GLFW_KEY_LEFT_CONTROL) || down(GLFW_KEY_RIGHT_CONTROL);
  state.shift = down(GLFW_KEY_LEFT_SHIFT) || down(GLFW_KEY_RIGHT_SHIFT);
  for_each_sources(bindings, [&](const ActionSources& sources) {
    for (const KeyboardChord& chord : sources.keys) {
      const int glfw_key = glfw_key_named(chord.key);
      if (glfw_key < 0 || !down(glfw_key)) {
        continue;
      }
      bool already = false;
      for (const std::string& held : state.keys_down) {
        if (held == chord.key) {
          already = true;
          break;
        }
      }
      if (!already) {
        state.keys_down.push_back(chord.key);
      }
    }
  });
  return state;
}

[[nodiscard]] GamepadState sample_gamepad() {
  GamepadState pad;
  GLFWgamepadstate raw{};
  if (glfwGetGamepadState(GLFW_JOYSTICK_1, &raw) != GLFW_TRUE) {
    return pad;
  }
  pad.connected = true;
  for (int i = 0; i < kGamepadButtonCount; ++i) {
    pad.buttons[static_cast<std::size_t>(i)] = raw.buttons[i] == GLFW_PRESS;
  }
  for (int i = 0; i < kGamepadAxisCount; ++i) {
    pad.axes[static_cast<std::size_t>(i)] = raw.axes[i];
  }
  return pad;
}

}  // namespace

NativeWindow::NativeWindow() : clock_(&owned_clock_) {}

NativeWindow::NativeWindow(const Clock& clock) : clock_(&clock) {}

NativeWindow::~NativeWindow() {
  destroy();
}

bool NativeWindow::create(int width, int height, const char* title) {
  if (window_ != nullptr) {
    return true;
  }
  if (!glfwInit()) {
    return false;
  }
  glfw_ready_ = true;
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  window_ = glfwCreateWindow(width, height, title != nullptr ? title : "rat-editor", nullptr,
                             nullptr);
  if (window_ == nullptr) {
    return false;
  }
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, glfw_framebuffer_size);
  return true;
}

void NativeWindow::destroy() {
  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  if (glfw_ready_) {
    glfwTerminate();
    glfw_ready_ = false;
  }
}

bool NativeWindow::is_open() const {
  return window_ != nullptr;
}

bool NativeWindow::should_close() const {
  return window_ == nullptr || glfwWindowShouldClose(window_) == GLFW_TRUE;
}

void NativeWindow::request_close() {
  if (window_ != nullptr) {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }
}

void NativeWindow::poll() {
  glfwPollEvents();
}

double NativeWindow::time() const {
  return clock_ != nullptr ? clock_->now_seconds() : 0.0;
}

InputButtons NativeWindow::sample_buttons() const {
  if (window_ == nullptr) {
    return {};
  }
  return merge_input_buttons(map_keyboard_buttons(sample_keyboard(window_, bindings_), bindings_),
                             map_gamepad_buttons(sample_gamepad(), bindings_));
}

void NativeWindow::set_input_bindings(InputBindings bindings) {
  bindings_ = std::move(bindings);
}

bool NativeWindow::mouse_left_down() const {
  return window_ != nullptr && glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

bool NativeWindow::mouse_right_down() const {
  return window_ != nullptr && glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

bool NativeWindow::key_escape_down() const {
  return window_ != nullptr && glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

void NativeWindow::cursor_pos(double& x, double& y) const {
  if (window_ == nullptr) {
    x = 0.0;
    y = 0.0;
    return;
  }
  glfwGetCursorPos(window_, &x, &y);
}

void NativeWindow::framebuffer_size(int& width, int& height) const {
  if (window_ == nullptr) {
    width = 1;
    height = 1;
    return;
  }
  glfwGetFramebufferSize(window_, &width, &height);
}

NativeWindowHandle NativeWindow::handle() const {
  NativeWindowHandle native;
  if (window_ == nullptr) {
    return native;
  }
#if defined(_WIN32)
  native.nwh = glfwGetWin32Window(window_);
#elif defined(__linux__)
  native.ndt = glfwGetX11Display();
  native.nwh = reinterpret_cast<void*>(static_cast<std::uintptr_t>(glfwGetX11Window(window_)));
#else
#error Native window handles are only wired for Win32 and Linux X11
#endif
  return native;
}

void NativeWindow::set_user_pointer(void* user) {
  user_ = user;
}

void NativeWindow::set_resize_callback(void (*callback)(void* user, int width, int height)) {
  resize_ = callback;
}

void NativeWindow::glfw_framebuffer_size(GLFWwindow* window, int width, int height) {
  auto* self = static_cast<NativeWindow*>(glfwGetWindowUserPointer(window));
  if (self != nullptr && self->resize_ != nullptr) {
    self->resize_(self->user_, width, height);
  }
}

}  // namespace rat
