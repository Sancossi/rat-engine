#pragma once

#include <rat/app_mode.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/player.hpp>

#include <string>

struct GLFWwindow;

namespace rat {

class Engine;

class EditorApp {
 public:
  EditorApp() = default;
  ~EditorApp();

  EditorApp(const EditorApp&) = delete;
  EditorApp& operator=(const EditorApp&) = delete;

  bool init();
  int run();

 private:
  void shutdown();
  void on_framebuffer_resize(int width, int height);
  void update_simulation(float dt);
  void draw_ui();
  void refresh_mode_banner();
  bool hot_apply_map_path(const std::string& path, bool preserve_player);
  void sync_blockers_to_runtime();
  void sync_events_to_runtime();
  void draw_blocker_edit_ui();
  void draw_event_edit_ui();

  static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

  GLFWwindow* window_ = nullptr;
  Engine* engine_ = nullptr;
  PlayerBody player_{};
  GameState game_state_{};
  EventRuntime events_{};
  AppMode app_mode_ = AppMode::Play;
  std::string map_path_;
  std::string last_apply_error_;
  std::string last_serialize_status_;
  int selected_blocker_ = -1;
  int selected_event_ = -1;
  int next_stub_event_ = 1;
  int width_ = 1280;
  int height_ = 720;
  bool running_ = false;
  bool interact_was_down_ = false;
  bool camera_toggle_was_down_ = false;
  bool mode_toggle_was_down_ = false;
  bool hot_apply_was_down_ = false;
  double last_time_ = 0.0;
};

}  // namespace rat
