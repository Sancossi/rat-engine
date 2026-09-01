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

namespace rat {

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
  InputButtons buttons;
  if (window_ == nullptr) {
    return buttons;
  }
  const auto down = [this](int key) { return glfwGetKey(window_, key) == GLFW_PRESS; };
  buttons.move_up = down(GLFW_KEY_W) || down(GLFW_KEY_UP);
  buttons.move_down = down(GLFW_KEY_S) || down(GLFW_KEY_DOWN);
  buttons.move_left = down(GLFW_KEY_A) || down(GLFW_KEY_LEFT);
  buttons.move_right = down(GLFW_KEY_D) || down(GLFW_KEY_RIGHT);
  buttons.jump = down(GLFW_KEY_SPACE);
  buttons.interact = down(GLFW_KEY_E);
  buttons.toggle_mode = down(GLFW_KEY_F2);
  buttons.hot_apply = down(GLFW_KEY_F5);
  buttons.cycle_camera = down(GLFW_KEY_C);
  buttons.debug_snapshot = down(GLFW_KEY_F3);
  const bool ctrl = down(GLFW_KEY_LEFT_CONTROL) || down(GLFW_KEY_RIGHT_CONTROL);
  const bool shift = down(GLFW_KEY_LEFT_SHIFT) || down(GLFW_KEY_RIGHT_SHIFT);
  const bool z = down(GLFW_KEY_Z);
  buttons.undo = ctrl && z && !shift;
  buttons.redo = ctrl && (down(GLFW_KEY_Y) || (shift && z));
  return buttons;
}

bool NativeWindow::mouse_left_down() const {
  return window_ != nullptr && glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
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
