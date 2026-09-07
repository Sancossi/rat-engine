#pragma once

#include "editor_document.hpp"
#include "editor_launch_options.hpp"
#include "editor_action_controller.hpp"
#include "frame_coordinator.hpp"
#include "panels/blocker_panel.hpp"
#include "panels/event_panel.hpp"
#include "panels/terrain_panel.hpp"
#include "platform/native_window.hpp"

#include <rat/app_mode.hpp>
#include <rat/asset.hpp>
#include <rat/audio.hpp>
#include <rat/debug_snapshot.hpp>
#include <rat/gameplay_notify.hpp>
#include <rat/file_store.hpp>
#include <rat/input.hpp>
#include <rat/log.hpp>
#include <rat/simulation_session.hpp>
#include <rat/viewport_edit.hpp>

#include <memory>
#include <string>

struct ImGuiIO;

namespace rat {

class Engine;

class EditorApp {
 public:
  explicit EditorApp(FileStore& files = os_files()) : files_(&files) {}
  ~EditorApp();

  EditorApp(const EditorApp&) = delete;
  EditorApp& operator=(const EditorApp&) = delete;

  bool init(const EditorLaunchOptions& options);
  int run();

 private:
  void shutdown();
  void on_framebuffer_resize(int width, int height);
  void simulate(float dt);
  void begin_ui();
  void draw_ui();
  void present();
  void refresh_mode_banner();
  void set_app_mode(AppMode next_mode);
  bool apply_edited_map(bool preserve_player);
  bool hot_apply_map_path(const std::string& path, bool preserve_player);
  bool save_map_path(const std::string& path);
  void sync_authoring_to_engine();
  void sync_selection_to_engine();
  void handle_edit_mouse_input(const ImGuiIO& io);
  void snap_player_to_ground_clear_jump();
  void run_drag_step_commands(const ViewportPick& pick, TileDelta delta);
  void apply_cell_brush(const ViewportClickAction& action);
  void finish_cell_brush(bool abort);
  void bind_session_assets();
  void save_play_slot();
  void load_play_slot(bool backup = false);
  void request_map_action(EditorActionKind kind, const std::string& path, bool preserve_player);
  bool install_authoring_map(MapData candidate, const std::string& path, bool preserve_player, bool restore);
  EditorActionResult settle_authoring();
  void reset_authoring_input();
  void draw_unsaved_modal();

  static void on_host_resize(void* user, int width, int height);

  EditorLaunchOptions launch_options_;
  FileStore* files_ = nullptr;
  NativeWindow host_{};
  FrameCoordinator coordinator_{};
  Engine* engine_ = nullptr;
  std::unique_ptr<FileLogSink> file_log_;
  std::unique_ptr<StreamLogSink> stderr_log_;
  std::unique_ptr<TeeLogSink> tee_log_;
  std::unique_ptr<Logger> logger_;
  std::unique_ptr<FileAssetLoader> asset_loader_;
  std::unique_ptr<AssetRegistry> asset_registry_;
  std::unique_ptr<AudioSink> audio_sink_;
  std::unique_ptr<QueuedAudio> audio_;
  SimulationSession session_{};
  EditorDocument document_{};
  EditorActionController actions_{};
  bool runtime_valid_ = false;
  GameplayNotifyBus notify_bus_{};
  BlockerPanelState blocker_panel_{};
  TerrainPanelState terrain_panel_{};
  EventPanelState event_panel_{};
  AppMode app_mode_ = AppMode::Play;
  std::string map_path_;
  std::string open_map_path_;
  std::string last_apply_error_;
  std::string last_serialize_status_;
  ViewportTool viewport_tool_ = ViewportTool::Select;
  EditSubmode edit_submode_{EditSubmode::Terrain};
  int fence_preset_index_ = 0;
  bool mouse_left_was_down_ = false;
  bool mouse_right_was_down_ = false;
  bool event_context_open_ = false;
  int event_context_index_ = -1;
  TileCoord event_context_tile_{};
  bool event_graph_open_{false};
  bool drag_active_ = false;
  bool brush_active_ = false;
  ViewportPick drag_pick_{};
  TileCoord drag_last_tile_{};
  RampDirection drag_last_edge_{};
  int drag_last_voxel_y_ = 0;
  int width_ = 1280;
  int height_ = 720;
  bool running_ = false;
  InputButtons previous_buttons_{};
  float fixed_accumulator_ = 0.0f;
  double last_time_ = 0.0;
  bool play_paused_ = false;
  bool inventory_open_ = false;
  bool escape_was_down_ = false;
  bool i_was_down_ = false;
  std::string last_save_status_;
};

}  // namespace rat
