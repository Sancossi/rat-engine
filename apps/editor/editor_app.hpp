#pragma once

#include <rat/app_mode.hpp>
#include <rat/buffered_press.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <memory>
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
  void set_app_mode(AppMode next_mode);
  bool hot_apply_map_path(const std::string& path, bool preserve_player);
  bool save_map_path(const std::string& path);
  void sync_blockers_to_runtime();
  void sync_events_to_runtime();
  void draw_blocker_edit_ui();
  void draw_event_edit_ui();
  void snap_player_to_ground_clear_jump();
  void rebuild_surface_query_cache();

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
  int selected_page_ = 0;
  int next_stub_event_ = 1;
  int width_ = 1280;
  int height_ = 720;
  bool running_ = false;
  bool interact_was_down_ = false;
  BufferedPress interact_press_buffer_{};
  bool jump_was_down_ = false;
  bool jump_press_pending_ = false;
  bool camera_toggle_was_down_ = false;
  bool mode_toggle_was_down_ = false;
  bool hot_apply_was_down_ = false;
  JumpState jump_state_ = make_grounded_jump_state();
  JumpTuning jump_tuning_{};
  std::unique_ptr<SurfaceQuery> surface_query_cache_;
  float fixed_accumulator_ = 0.0f;
  double last_time_ = 0.0;
};

}  // namespace rat
