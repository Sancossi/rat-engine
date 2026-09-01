#pragma once

#include <rat/clock.hpp>
#include <rat/input.hpp>
#include <rat/input_bindings.hpp>
#include <rat/native_window_handle.hpp>

struct GLFWwindow;

namespace rat {

class NativeWindow {
 public:
  NativeWindow();
  explicit NativeWindow(const Clock& clock);
  ~NativeWindow();

  NativeWindow(const NativeWindow&) = delete;
  NativeWindow& operator=(const NativeWindow&) = delete;

  bool create(int width, int height, const char* title);
  void destroy();

  [[nodiscard]] bool is_open() const;
  [[nodiscard]] bool should_close() const;
  void request_close();
  void poll();
  [[nodiscard]] double time() const;
  [[nodiscard]] InputButtons sample_buttons() const;
  void set_input_bindings(InputBindings bindings);
  [[nodiscard]] const InputBindings& input_bindings() const { return bindings_; }
  [[nodiscard]] bool mouse_left_down() const;
  void cursor_pos(double& x, double& y) const;
  void framebuffer_size(int& width, int& height) const;
  [[nodiscard]] NativeWindowHandle handle() const;

  // ImGui GLFW backend still needs the GLFW window; not for simulation.
  [[nodiscard]] GLFWwindow* glfw_window() const { return window_; }

  void set_user_pointer(void* user);
  void set_resize_callback(void (*callback)(void* user, int width, int height));

 private:
  static void glfw_framebuffer_size(GLFWwindow* window, int width, int height);

  SteadyClock owned_clock_{};
  const Clock* clock_ = nullptr;
  InputBindings bindings_ = default_input_bindings();
  GLFWwindow* window_ = nullptr;
  void* user_ = nullptr;
  void (*resize_)(void* user, int width, int height) = nullptr;
  bool glfw_ready_ = false;
};

}  // namespace rat
