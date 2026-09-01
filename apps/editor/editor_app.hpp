#pragma once

#include <rat/app_mode.hpp>
#include <rat/audio.hpp>
#include <rat/debug_snapshot.hpp>
#include <rat/edit_history.hpp>
#include <rat/gameplay_notify.hpp>
#include <rat/input.hpp>
#include <rat/log.hpp>
#include <rat/map_document.hpp>
#include <rat/simulation_session.hpp>
#include <rat/viewport_edit.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct GLFWwindow;
struct ImGuiIO;

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
  void draw_height_edit_ui();
  void draw_event_edit_ui();
  void handle_edit_mouse_input(const ImGuiIO& io);
  bool run_height_history(std::unique_ptr<EditCommand> command);
  void sync_height_tile_from_click(TileCoord tile);
  void snap_player_to_ground_clear_jump();
  void rebuild_surface_query_cache();
  void apply_edited_map(MapData map, EditApplyResult mutation);
  void discard_field_edit_origins();
  void execute_edit_command(std::unique_ptr<EditCommand> command);
  void clear_map_selection();
  void select_blocker_from_map(int index);
  void select_event_from_map(int index);
  void run_drag_step_commands(const ViewportPick& pick, TileDelta delta);

  static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

  GLFWwindow* window_ = nullptr;
  Engine* engine_ = nullptr;
  std::unique_ptr<FileLogSink> file_log_;
  std::unique_ptr<StreamLogSink> stderr_log_;
  std::unique_ptr<TeeLogSink> tee_log_;
  std::unique_ptr<Logger> logger_;
  std::unique_ptr<LogAudioSink> audio_sink_;
  std::unique_ptr<QueuedAudio> audio_;
  SimulationSession session_{};
  MapDocument document_{};
  GameplayNotifyBus notify_bus_{};
  EditHistory edit_history_{};
  AppMode app_mode_ = AppMode::Play;
  std::string map_path_;
  std::string last_apply_error_;
  std::string last_serialize_status_;
  int selected_blocker_ = -1;
  int selected_event_ = -1;
  int selected_page_ = 0;
  ViewportTool viewport_tool_ = ViewportTool::Select;
  int fence_preset_index_ = 0;
  bool mouse_left_was_down_ = false;
  bool drag_active_ = false;
  ViewportPick drag_pick_{};
  TileCoord drag_last_tile_{};
  std::optional<BlockerDef> blocker_field_origin_{};
  int blocker_field_origin_index_ = -1;
  std::optional<EventDef> event_field_origin_{};
  int event_field_origin_index_ = -1;
  int height_tile_x_ = 0;
  int height_tile_z_ = 0;
  float height_step_ = 0.25f;
  float height_set_y_ = 0.0f;
  int ramp_direction_index_ = 0;
  int edge_direction_index_ = 1;
  float ramp_low_y_ = 0.0f;
  float ramp_high_y_ = 0.0f;
  bool height_tile_sync_ready_ = false;
  int last_height_tile_x_ = 0;
  int last_height_tile_z_ = 0;
  int next_stub_event_ = 1;
  int width_ = 1280;
  int height_ = 720;
  bool running_ = false;
  InputButtons previous_buttons_{};
  float fixed_accumulator_ = 0.0f;
  double last_time_ = 0.0;
};

}  // namespace rat
