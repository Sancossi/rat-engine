#pragma once

#include <rat/input.hpp>

struct GLFWwindow;

namespace rat {

struct NativeWindow {
  void* nwh = nullptr;
  void* ndt = nullptr;
};

class GlfwHost {
 public:
  GlfwHost() = default;
  ~GlfwHost();

  GlfwHost(const GlfwHost&) = delete;
  GlfwHost& operator=(const GlfwHost&) = delete;

  bool create(int width, int height, const char* title);
  void destroy();

  [[nodiscard]] bool should_close() const;
  void request_close();
  void poll();
  [[nodiscard]] double time() const;
  [[nodiscard]] InputButtons sample_buttons() const;
  [[nodiscard]] bool mouse_left_down() const;
  void cursor_pos(double& x, double& y) const;
  void framebuffer_size(int& width, int& height) const;
  [[nodiscard]] NativeWindow native_window() const;

  [[nodiscard]] GLFWwindow* window() const { return window_; }
  void set_user_pointer(void* user);
  void set_framebuffer_size_callback(void (*callback)(GLFWwindow*, int, int));

 private:
  GLFWwindow* window_ = nullptr;
  bool glfw_ready_ = false;
};

}  // namespace rat
