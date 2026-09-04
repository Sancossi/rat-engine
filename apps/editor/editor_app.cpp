#include "editor_app.hpp"

#include "imgui_bgfx.hpp"
#include "panels/event_graph_window.hpp"
#include "panels/ladder_panel.hpp"
#include "platform/miniaudio_sink.hpp"

#include <rat/debug_snapshot.hpp>
#include <rat/edit_history.hpp>
#include <rat/engine.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_runtime.hpp>
#include <rat/file_store.hpp>
#include <rat/height_edit.hpp>
#include <rat/hot_apply.hpp>
#include <rat/input.hpp>
#include <rat/inventory_list.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>
#include <rat/replay.hpp>
#include <rat/render_world.hpp>
#include <rat/save_game.hpp>
#include <rat/simulation_session.hpp>
#include <rat/surface_query.hpp>
#include <rat/viewport_edit.hpp>

#include <algorithm>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace rat {

namespace {

constexpr const char* kPlaySaveSlotPath = "saves/slot1.ratsave";
constexpr float kEventMarkerStemHeight = 1.4f;
constexpr float kUiFontSizePx = 16.0f;

void try_load_cyrillic_ui_font(ImGuiIO& io, Logger& logger) {
  const std::string path = std::string(RAT_DATA_DIR) + "/fonts/NotoSans-Regular.ttf";
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec)) {
    log(logger, LogLevel::Error, "editor",
        std::string("UI font missing, using default: ") + path);
    return;
  }
  ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), kUiFontSizePx, nullptr,
                                              io.Fonts->GetGlyphRangesCyrillic());
  if (font == nullptr) {
    log(logger, LogLevel::Error, "editor",
        std::string("Failed to load UI font, using default: ") + path);
  }
}

}  // namespace

EditorApp::~EditorApp() {
  shutdown();
}

void EditorApp::sync_selection_to_engine() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->greybox().set_selected_blocker(app_mode_ == AppMode::Edit ? document_.selected_blocker()
                                                                     : -1);
  const int marker = app_mode_ == AppMode::Edit
                         ? event_marker_index(document_.visible_data(), document_.selected_event())
                         : -1;
  engine_->greybox().set_selected_event_marker(marker);
}

void EditorApp::sync_authoring_to_engine() {
  if (engine_ == nullptr) {
    return;
  }
  if (!document_.consume_changed()) {
    sync_selection_to_engine();
    return;
  }
  const MapCompileResult compiled = compile_map_data(document_.visible_data());
  if (!compiled.ok) {
    last_apply_error_ = format_map_issues(compiled.issues);
    sync_selection_to_engine();
    return;
  }
  last_apply_error_.clear();
  session_.events().load(compiled.runtime);
  session_.rebuild_surface();
  engine_->set_terrain_map(session_.events().map());
  engine_->set_blockers(session_.events().map().blockers);
  engine_->set_event_markers(event_markers_from_map(session_.events().map()));
  sync_selection_to_engine();
  if (document_.last_mutated_elevation()) {
    snap_player_to_ground_clear_jump();
  }
}

void EditorApp::run_drag_step_commands(const ViewportPick& pick, TileDelta delta) {
  const float tile =
      document_.visible_data().tile_size > 0.0f ? document_.visible_data().tile_size : 1.0f;
  while (delta.tile_dx != 0 || delta.tile_dz != 0) {
    const int step_x = delta.tile_dx > 0 ? 1 : (delta.tile_dx < 0 ? -1 : 0);
    const int step_z = delta.tile_dz > 0 ? 1 : (delta.tile_dz < 0 ? -1 : 0);
    if (pick.kind == ViewportPickKind::Blocker) {
      (void)document_.execute(make_move_blocker_command(pick.index, step_x, step_z, tile));
      document_.select_blocker(static_cast<int>(pick.index));
    } else {
      (void)document_.execute(make_move_event_command(pick.index, step_x, step_z, tile));
      document_.select_event(static_cast<int>(pick.index));
    }
    delta.tile_dx -= step_x;
    delta.tile_dz -= step_z;
  }
}

void EditorApp::apply_cell_brush(const ViewportClickAction& action) {
  switch (action.kind) {
    case ViewportClickActionKind::PlaceCube: {
      terrain_panel_.tile_x = action.tile.x;
      terrain_panel_.tile_z = action.tile.z;
      if (document_.execute(make_place_map_tile_cube_command(action.tile.x, action.tile.z))) {
        const HeightGetResult updated =
            get_tile_ground_y(document_.data().height_grid, terrain_panel_.tile_x,
                              terrain_panel_.tile_z);
        if (updated.ok) {
          terrain_panel_.set_y = updated.value;
        }
      }
      break;
    }
    case ViewportClickActionKind::PlaceSlab: {
      terrain_panel_.tile_x = action.tile.x;
      terrain_panel_.tile_z = action.tile.z;
      FloorSlabDef slab;
      slab.tile = action.tile;
      slab.top_y = terrain_panel_.slab_top_y;
      slab.thickness = terrain_panel_.slab_thickness;
      (void)document_.execute(make_upsert_map_floor_slab_command(std::move(slab)));
      break;
    }
    case ViewportClickActionKind::PlaceFence: {
      terrain_panel_.tile_x = action.tile.x;
      terrain_panel_.tile_z = action.tile.z;
      terrain_panel_.edge_direction_index = static_cast<int>(action.edge);
      if (fence_preset_index_ == 2) {
        (void)document_.execute(make_remove_map_edge_barrier_command(action.tile, action.edge));
      } else {
        EdgeBarrierDef edge;
        edge.tile = action.tile;
        edge.direction = action.edge;
        edge.height = fence_preset_index_ == 1 ? kEdgeBarrierFullHeight : kEdgeBarrierMiniHeight;
        (void)document_.execute(make_upsert_map_edge_barrier_command(std::move(edge)));
      }
      break;
    }
    case ViewportClickActionKind::PlaceLadder: {
      terrain_panel_.tile_x = action.tile.x;
      terrain_panel_.tile_z = action.tile.z;
      terrain_panel_.edge_direction_index = static_cast<int>(action.edge);
      LadderDef ladder;
      ladder.tile = action.tile;
      ladder.direction = action.edge;
      ladder.y_lo = terrain_panel_.ladder_y_lo;
      ladder.y_hi = terrain_panel_.ladder_y_hi;
      (void)document_.execute(make_upsert_map_ladder_command(std::move(ladder)));
      break;
    }
    default:
      break;
  }
}

void EditorApp::finish_cell_brush(bool abort) {
  if (abort) {
    (void)document_.abort_stroke();
  } else {
    document_.end_stroke();
  }
  brush_active_ = false;
}

bool EditorApp::hot_apply_map_path(const std::string& path, bool preserve_player) {
  if (engine_ == nullptr) {
    last_apply_error_ = "engine not ready";
    return false;
  }

  std::vector<BlockerDef> blockers;
  std::vector<Vec3> markers;
  HotApplyTargets targets{session_.events(), session_.state(), session_.player(), blockers, markers,
                          &session_.surface_query(), &session_.jump()};
  HotApplyOptions options;
  options.preserve_player_position = preserve_player;

  const HotApplyResult result = hot_apply_map_from_file(path, targets, options);
  if (!result.ok) {
    last_apply_error_ = result.error;
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("hot-apply failed (") + path + "): " + result.error);
    }
    return false;
  }

  last_apply_error_.clear();
  map_path_ = path;
  document_.load(session_.events().map());
  (void)document_.consume_changed();
  terrain_panel_.tile_x = session_.events().map().height_grid.origin_x;
  terrain_panel_.tile_z = session_.events().map().height_grid.origin_z;
  terrain_panel_.step = 0.25f;
  terrain_panel_.set_y = 0.0f;
  terrain_panel_.ramp_direction_index = 0;
  terrain_panel_.ramp_low_y = 0.0f;
  terrain_panel_.ramp_high_y = 0.0f;
  terrain_panel_.tile_sync_ready = false;
  terrain_panel_.last_tile_x = 0;
  terrain_panel_.last_tile_z = 0;
  blocker_panel_ = {};
  event_panel_.field_origin.reset();
  event_panel_.field_origin_index = -1;
  event_panel_.canvas = {};
  event_panel_.last_compile_error.clear();
  engine_->set_terrain_map(session_.events().map());
  engine_->set_blockers(std::move(blockers));
  engine_->set_event_markers(std::move(markers));
  engine_->set_player(session_.player());
  sync_selection_to_engine();
  bind_session_assets();
  session_.set_app_mode(app_mode_);
  session_.clear_pending_input();
  previous_buttons_ = host_.sample_buttons();
  fixed_accumulator_ = 0.0f;
  snap_player_to_ground_clear_jump();
  return true;
}

bool EditorApp::save_map_path(const std::string& path) {
  if (path.empty()) {
    last_apply_error_ = "map path is empty";
    return false;
  }
  const EventGraphApplyResult compiled = document_.compile_graphs_for_apply();
  if (!compiled.ok) {
    last_apply_error_ = compiled.issues.empty() ? "event graph compile failed"
                                               : format_map_issues(compiled.issues);
    last_serialize_status_.clear();
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("map save rejected (graph compile): ") + last_apply_error_);
    }
    return false;
  }
  const MapFileResult result = save_map_to_file(document_.data(), path);
  if (!result.ok) {
    last_apply_error_ = result.error;
    last_serialize_status_.clear();
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("map save failed (") + path + "): " + result.error);
    }
    return false;
  }
  last_apply_error_.clear();
  last_serialize_status_ = "Saved: " + path;
  document_.mark_clean();
  event_panel_.last_compile_error.clear();
  return true;
}

void EditorApp::save_play_slot() {
  std::error_code ec;
  std::filesystem::create_directories("saves", ec);
  if (ec) {
    last_save_status_ = "failed to create saves directory: " + ec.message();
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "save", last_save_status_);
    }
    return;
  }
  const GameFileResult result = save_game(os_files(), kPlaySaveSlotPath, session_.state());
  if (!result.ok) {
    last_save_status_ = result.error.empty() ? "Save failed" : result.error;
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "save", last_save_status_);
    }
    return;
  }
  last_save_status_ = std::string("Saved ") + kPlaySaveSlotPath;
  if (logger_ != nullptr) {
    log(*logger_, LogLevel::Info, "save", last_save_status_);
  }
}

void EditorApp::load_play_slot() {
  GameState loaded;
  const GameFileResult result = load_game(os_files(), kPlaySaveSlotPath, loaded);
  if (!result.ok) {
    last_save_status_ = result.error.empty() ? "Load failed" : result.error;
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "save", last_save_status_);
    }
    return;
  }
  const GameFileResult applied = session_.apply_loaded_game(loaded);
  if (!applied.ok) {
    last_save_status_ = applied.error.empty() ? "Load failed" : applied.error;
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "save", last_save_status_);
    }
    return;
  }
  if (engine_ != nullptr) {
    engine_->set_player(session_.player());
  }
  last_save_status_ = std::string("Loaded ") + kPlaySaveSlotPath;
  if (logger_ != nullptr) {
    log(*logger_, LogLevel::Info, "save", last_save_status_);
  }
}

bool EditorApp::apply_edited_map(bool preserve_player) {
  if (engine_ == nullptr) {
    last_apply_error_ = "engine not ready";
    return false;
  }
  const EventGraphApplyResult compiled = document_.compile_graphs_for_apply();
  if (!compiled.ok) {
    last_apply_error_ = compiled.issues.empty() ? "event graph compile failed"
                                               : format_map_issues(compiled.issues);
    event_panel_.last_compile_error = last_apply_error_;
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("apply rejected (graph compile): ") + last_apply_error_);
    }
    return false;
  }
  event_panel_.last_compile_error.clear();

  std::vector<BlockerDef> blockers;
  std::vector<Vec3> markers;
  HotApplyTargets targets{session_.events(), session_.state(), session_.player(), blockers, markers,
                          &session_.surface_query(), &session_.jump()};
  HotApplyOptions options;
  options.preserve_player_position = preserve_player;
  const HotApplyResult result = hot_apply_map(document_.data(), targets, options);
  if (!result.ok) {
    last_apply_error_ = result.error;
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("apply edited map failed: ") + result.error);
    }
    return false;
  }

  last_apply_error_.clear();
  engine_->set_terrain_map(session_.events().map());
  engine_->set_blockers(std::move(blockers));
  engine_->set_event_markers(std::move(markers));
  engine_->set_player(session_.player());
  sync_selection_to_engine();
  bind_session_assets();
  session_.set_app_mode(app_mode_);
  session_.clear_pending_input();
  previous_buttons_ = host_.sample_buttons();
  fixed_accumulator_ = 0.0f;
  snap_player_to_ground_clear_jump();
  return true;
}

void EditorApp::bind_session_assets() {
  if (asset_registry_ == nullptr) {
    return;
  }
  bind_map_assets(*asset_registry_, session_.events().map());
  const AssetId beep = make_asset_id("sfx/beep");
  AssetCatalogEntry overlay;
  overlay.id = beep;
  overlay.kind = AssetKind::AudioClip;
  overlay.debug_name = "beep";
  overlay.path.compiled = std::string(RAT_DATA_DIR) + "/audio/beep.wav";
  asset_registry_->register_asset(std::move(overlay));
  asset_registry_->request_load(beep);
  asset_registry_->pump_loads();
}

bool EditorApp::init() {
  file_log_ = std::make_unique<FileLogSink>(default_log_path());
  stderr_log_ = std::make_unique<StreamLogSink>(std::cerr);
  tee_log_ = std::make_unique<TeeLogSink>(*file_log_, *stderr_log_);
  logger_ = std::make_unique<Logger>(*tee_log_);
  asset_loader_ = std::make_unique<FileAssetLoader>(os_files());
  asset_registry_ = std::make_unique<AssetRegistry>(*asset_loader_);
  audio_sink_ = make_editor_audio_sink(*asset_registry_, *logger_);
  audio_ = std::make_unique<QueuedAudio>(*audio_sink_);
  session_.set_audio(audio_.get());
  session_.set_notify(&notify_bus_);
  notify_bus_.subscribe([this](const GameplayNotify& notify) {
    if (logger_ == nullptr) {
      return;
    }
    std::string message = gameplay_notify_kind_name(notify.kind);
    if (!notify.id.empty()) {
      message += ' ';
      message += notify.id;
    }
    log(*logger_, LogLevel::Info, "notify", message);
  });
  if (file_log_->ok()) {
    log(*logger_, LogLevel::Info, "editor", std::string("log file ") + file_log_->path());
  } else {
    log(*logger_, LogLevel::Warn, "editor",
        std::string("could not open log file ") + file_log_->path());
  }
  log(*logger_, LogLevel::Info, "editor", "starting rat-editor");

  if (!host_.create(width_, height_, "rat-editor")) {
    log(*logger_, LogLevel::Error, "editor", "NativeWindow::create failed");
    return false;
  }

  host_.set_user_pointer(this);
  host_.set_resize_callback(on_host_resize);
  host_.framebuffer_size(width_, height_);

  engine_ = new Engine();

  RendererConfig config;
  config.window = host_.handle();
  config.width = static_cast<std::uint32_t>(width_ > 0 ? width_ : 1);
  config.height = static_cast<std::uint32_t>(height_ > 0 ? height_ : 1);
  config.vsync = true;

  if (!engine_->init(config)) {
    log(*logger_, LogLevel::Error, "editor", "Engine::init failed");
    shutdown();
    return false;
  }
  engine_->set_debug_banner("rat-engine");

#ifndef RAT_DATA_DIR
#error RAT_DATA_DIR must be defined
#endif
  const std::string map_path = std::string(RAT_DATA_DIR) + "/maps/grey_yard.json";
  if (!hot_apply_map_path(map_path, false)) {
    log(*logger_, LogLevel::Error, "editor",
        std::string("Failed to load map ") + map_path + ": " + last_apply_error_);
    shutdown();
    return false;
  }

  session_.player().x = -1.5f;
  session_.player().z = 1.5f;
  snap_player_to_ground_clear_jump();

  refresh_mode_banner();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  try_load_cyrillic_ui_font(io, *logger_);

  if (!ImGui_ImplGlfw_InitForOther(host_.glfw_window(), true)) {
    log(*logger_, LogLevel::Error, "editor", "ImGui_ImplGlfw_InitForOther failed");
    shutdown();
    return false;
  }

  if (!imgui_bgfx::init(255)) {
    log(*logger_, LogLevel::Error, "editor", "imgui_bgfx::init failed");
    shutdown();
    return false;
  }

  coordinator_.poll = [this] { host_.poll(); };
  coordinator_.simulate = [this](float dt) { simulate(dt); };
  coordinator_.drain_audio = [this] {
    if (audio_ != nullptr) {
      audio_->drain();
    }
  };
  coordinator_.begin_ui = [this] { begin_ui(); };
  coordinator_.draw_ui = [this] { draw_ui(); };
  coordinator_.present = [this] { present(); };

  last_time_ = host_.time();
  running_ = true;
  return true;
}

int EditorApp::run() {
  if (!running_) {
    return 1;
  }

  while (!host_.should_close()) {
    const double now = host_.time();
    float dt = static_cast<float>(now - last_time_);
    last_time_ = now;
    if (dt < 0.0f) {
      dt = 0.0f;
    }
    if (dt > 0.1f) {
      dt = 0.1f;
    }
    coordinator_.run_frame(dt);
  }

  shutdown();
  return 0;
}

void EditorApp::shutdown() {
  if (!running_ && !host_.is_open() && engine_ == nullptr) {
    return;
  }
  running_ = false;

  imgui_bgfx::shutdown();
  ImGui_ImplGlfw_Shutdown();
  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui::DestroyContext();
  }

  if (engine_ != nullptr) {
    engine_->shutdown();
    delete engine_;
    engine_ = nullptr;
  }

  host_.destroy();
}

void EditorApp::on_framebuffer_resize(int width, int height) {
  width_ = width > 0 ? width : 1;
  height_ = height > 0 ? height : 1;
  if (engine_ != nullptr && engine_->is_initialized()) {
    engine_->resize(static_cast<std::uint32_t>(width_), static_cast<std::uint32_t>(height_));
  }
}

void EditorApp::refresh_mode_banner() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->set_debug_banner(std::string("rat-engine  [") + app_mode_name(app_mode_) + "]");
}

void EditorApp::set_app_mode(AppMode next_mode) {
  if (app_mode_ == next_mode) {
    return;
  }
  blocker_panel_.field_origin.reset();
  blocker_panel_.field_origin_index = -1;
  event_panel_.field_origin.reset();
  event_panel_.field_origin_index = -1;
  document_.discard_preview();
  app_mode_ = next_mode;
  session_.set_app_mode(next_mode);
  if (next_mode != AppMode::Play) {
    play_paused_ = false;
    inventory_open_ = false;
  }
  refresh_mode_banner();
  sync_selection_to_engine();
}

void EditorApp::snap_player_to_ground_clear_jump() {
  session_.reset_jump_grounded();
  if (engine_ != nullptr) {
    engine_->set_player(session_.player());
  }
}

void EditorApp::on_host_resize(void* user, int width, int height) {
  auto* self = static_cast<EditorApp*>(user);
  if (self != nullptr) {
    self->on_framebuffer_resize(width, height);
  }
}

void EditorApp::handle_edit_mouse_input(const ImGuiIO& io) {
  if (!host_.is_open() || engine_ == nullptr || app_mode_ != AppMode::Edit) {
    finish_cell_brush(false);
    mouse_left_was_down_ = false;
    mouse_right_was_down_ = false;
    drag_active_ = false;
    return;
  }

  const bool left_down = host_.mouse_left_down();
  const bool right_down = host_.mouse_right_down();
  const bool right_pressed = right_down && !mouse_right_was_down_;
  const bool aborting_brush = (brush_active_ || document_.in_stroke()) &&
                              (right_down || host_.key_escape_down());
  if (aborting_brush) {
    finish_cell_brush(true);
  }
  if (brush_active_ && !document_.in_stroke()) {
    brush_active_ = false;
  }

  auto remember_buttons = [&] {
    mouse_left_was_down_ = left_down;
    mouse_right_was_down_ = right_down;
  };

  if (io.WantCaptureMouse) {
    finish_cell_brush(false);
    remember_buttons();
    if (!left_down) {
      drag_active_ = false;
    }
    return;
  }

  double cursor_x = 0.0;
  double cursor_y = 0.0;
  host_.cursor_pos(cursor_x, cursor_y);
  const MapData& map = document_.visible_data();
  const TerrainGeometry terrain =
      build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  const auto world_hit =
      unproject_to_terrain(engine_->greybox().camera(), static_cast<float>(cursor_x),
                           static_cast<float>(cursor_y),
                           static_cast<std::uint32_t>(width_ > 0 ? width_ : 1),
                           static_cast<std::uint32_t>(height_ > 0 ? height_ : 1), terrain);
  if (!world_hit.has_value()) {
    remember_buttons();
    if (!left_down) {
      drag_active_ = false;
      finish_cell_brush(false);
    }
    return;
  }

  const bool pressed = left_down && !mouse_left_was_down_;
  const bool released = !left_down && mouse_left_was_down_;

  if (right_pressed && !aborting_brush && edit_submode_ == EditSubmode::Events) {
    event_context_tile_ = world_to_tile_xz(*world_hit, map.tile_size);
    if (const auto picked = pick_map_object_xz(map, *world_hit, EditSubmode::Events);
        picked.has_value() && picked->kind == ViewportPickKind::Event) {
      event_context_index_ = static_cast<int>(picked->index);
      document_.select_event(event_context_index_);
    } else {
      event_context_index_ = -1;
    }
    event_context_open_ = true;
  }

  if (pressed) {
    drag_active_ = false;
    const ViewportClickAction action =
        resolve_viewport_click(map, viewport_tool_, *world_hit, edit_submode_);
    switch (action.kind) {
      case ViewportClickActionKind::Deselect:
        document_.clear_selection();
        break;
      case ViewportClickActionKind::SelectBlocker:
        document_.select_blocker(static_cast<int>(action.index));
        drag_pick_ = {ViewportPickKind::Blocker, action.index};
        drag_last_tile_ = world_to_tile_xz(*world_hit, map.tile_size);
        drag_active_ = true;
        break;
      case ViewportClickActionKind::SelectEvent:
        document_.select_event(static_cast<int>(action.index));
        drag_pick_ = {ViewportPickKind::Event, action.index};
        drag_last_tile_ = world_to_tile_xz(*world_hit, map.tile_size);
        drag_active_ = true;
        break;
      case ViewportClickActionKind::PlaceBlocker: {
        const float tile = map.tile_size > 0.0f ? map.tile_size : 1.0f;
        BlockerDef blocker;
        blocker.bounds = {
            static_cast<float>(action.tile.x) * tile,
            static_cast<float>(action.tile.z) * tile,
            static_cast<float>(action.tile.x + 1) * tile,
            static_cast<float>(action.tile.z + 1) * tile,
        };
        (void)document_.execute(make_place_blocker_command(std::move(blocker)));
        document_.select_blocker(static_cast<int>(document_.data().blockers.size()) - 1);
        break;
      }
      case ViewportClickActionKind::PlaceEvent: {
        const std::string id = "stub_" + std::to_string(event_panel_.next_stub_event++);
        (void)document_.execute(make_place_event_command(make_stub_event(id, action.tile.x, action.tile.z)));
        document_.select_event(static_cast<int>(document_.data().events.size()) - 1);
        break;
      }
      case ViewportClickActionKind::PlaceCube:
      case ViewportClickActionKind::PlaceSlab:
      case ViewportClickActionKind::PlaceFence:
      case ViewportClickActionKind::PlaceLadder:
        document_.begin_stroke();
        apply_cell_brush(action);
        brush_active_ = true;
        drag_last_tile_ = action.tile;
        drag_last_edge_ = action.edge;
        break;
      case ViewportClickActionKind::None:
        break;
    }
  }

  if (left_down && brush_active_) {
    const ViewportClickAction action =
        resolve_viewport_click(map, viewport_tool_, *world_hit, edit_submode_);
    const bool cell_tool = action.kind == ViewportClickActionKind::PlaceCube ||
                           action.kind == ViewportClickActionKind::PlaceSlab;
    const bool edge_tool = action.kind == ViewportClickActionKind::PlaceFence ||
                           action.kind == ViewportClickActionKind::PlaceLadder;
    const bool tile_changed =
        action.tile.x != drag_last_tile_.x || action.tile.z != drag_last_tile_.z;
    const bool edge_changed = action.edge != drag_last_edge_;
    if ((cell_tool && tile_changed) || (edge_tool && (tile_changed || edge_changed))) {
      apply_cell_brush(action);
      drag_last_tile_ = action.tile;
      drag_last_edge_ = action.edge;
    }
  }

  if (left_down && drag_active_) {
    const TileCoord tile = world_to_tile_xz(*world_hit, map.tile_size);
    const TileDelta delta = tile_delta_between(drag_last_tile_, tile);
    if (delta.tile_dx != 0 || delta.tile_dz != 0) {
      run_drag_step_commands(drag_pick_, delta);
      drag_last_tile_ = tile;
    }
  }

  if (released) {
    drag_active_ = false;
    finish_cell_brush(false);
  }
  remember_buttons();
}

void EditorApp::simulate(float dt) {
  if (engine_ == nullptr || !host_.is_open()) {
    return;
  }

  const ImGuiIO& io = ImGui::GetIO();

  const InputButtons buttons = host_.sample_buttons();
  InputGating gating;
  gating.player_control = player_control_enabled(app_mode_);
  gating.keyboard_captured = io.WantCaptureKeyboard;
  gating.dialog_open = session_.events().active_message().has_value();
  gating.player_input_blocked = session_.events().player_input_blocked();

  engine_->set_player(session_.player());
  if (session_.jump().climbing) {
    engine_->greybox().set_climb_lock(true, session_.jump().climb_into_x,
                                     session_.jump().climb_into_z);
  } else {
    engine_->greybox().set_climb_lock(false, 0.0f, 0.0f);
  }

  Vec3 input_eye = engine_->greybox().camera().eye;
  Vec3 input_focus = engine_->greybox().camera_focus();
  if (session_.jump().climbing) {
    const ClimbCameraPose climb_pose =
        climb_camera_pose({session_.player().x, session_.player().y, session_.player().z},
                          session_.jump().climb_into_x, session_.jump().climb_into_z);
    input_eye = climb_pose.eye;
    input_focus = climb_pose.focus;
  }

  const InputFrame input =
      map_input_frame(buttons, previous_buttons_, gating, input_eye, input_focus);
  previous_buttons_ = buttons;

  const bool escape_down = host_.key_escape_down();
  if (app_mode_ == AppMode::Play && escape_down && !escape_was_down_) {
    play_paused_ = !play_paused_;
    if (play_paused_) {
      inventory_open_ = false;
      session_.clear_pending_input();
      fixed_accumulator_ = 0.0f;
    }
  }
  escape_was_down_ = escape_down;

  const bool i_down = host_.key_i_down();
  if (app_mode_ == AppMode::Play && i_down && !i_was_down_ && !play_paused_) {
    inventory_open_ = !inventory_open_;
    if (inventory_open_) {
      session_.clear_pending_input();
      fixed_accumulator_ = 0.0f;
    }
  }
  i_was_down_ = i_down;

  if (input.toggle_mode_pressed) {
    set_app_mode(toggle_app_mode(app_mode_));
  }

  if (input.hot_apply_pressed && !map_path_.empty()) {
    session_.clear_pending_input();
    hot_apply_map_path(map_path_, true);
  }

  if (app_mode_ == AppMode::Edit) {
    if (input.undo_pressed) {
      (void)document_.undo();
    }
    if (input.redo_pressed) {
      (void)document_.redo();
    }
  }

  handle_edit_mouse_input(io);

  if (io.WantCaptureKeyboard) {
    session_.clear_pending_input();
  }

  if (input.cycle_camera_pressed) {
    engine_->greybox().set_camera_mode(next_camera_mode(engine_->greybox().camera_mode()));
  }

  if (input.debug_snapshot_pressed) {
    std::string_view selected_id{};
    const auto& event_list = document_.visible_data().events;
    if (document_.selected_event() >= 0 &&
        document_.selected_event() < static_cast<int>(event_list.size())) {
      selected_id = event_list[static_cast<std::size_t>(document_.selected_event())].id;
    }
    const RenderWorld world = capture_render_world(session_);
    FrameMetricsSources sources;
    sources.session = &session_;
    sources.render = &world;
    sources.assets = asset_registry_.get();
    sources.audio = audio_.get();
    const DebugSnapshot snapshot =
        make_debug_snapshot(session_.tick_id(), app_mode_, session_.player(), session_.jump(),
                            session_.events(), session_.state(), input.interact_pressed, selected_id,
                            input, runtime_checksum(session_, 0), collect_frame_metrics(sources));
    const std::string path = default_debug_snapshot_path();
    if (write_debug_snapshot(path, snapshot)) {
      if (logger_ != nullptr) {
        log(*logger_, LogLevel::Info, "debug", std::string("wrote snapshot ") + path);
      }
    } else if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "debug", std::string("failed to write snapshot ") + path);
    }
  }

  if (app_mode_ == AppMode::Play && (play_paused_ || inventory_open_)) {
    engine_->set_player(session_.player());
    engine_->greybox().tick(dt);
    return;
  }

  fixed_accumulator_ += std::max(0.0f, dt);
  const SimulationCatchUpResult catch_up =
      drain_simulation_catch_up(session_, fixed_accumulator_, input);
  if (catch_up.budget_exceeded && logger_ != nullptr) {
    log(*logger_, LogLevel::Warn, "sim", "simulation catch-up budget exceeded");
  }
  engine_->set_player(session_.player());
  if (app_mode_ == AppMode::Play) {
    engine_->set_event_markers(session_.events().event_markers());
  }
  engine_->greybox().tick(dt);
}

void EditorApp::begin_ui() {
  ImGui_ImplGlfw_NewFrame();
  imgui_bgfx::begin_frame(width_, height_);
}

void EditorApp::present() {
  sync_authoring_to_engine();
  const RenderWorld world = capture_render_world(session_);
  engine_->begin_frame();
  engine_->submit_world(world);
  imgui_bgfx::end_frame();
  engine_->end_frame();
}

void EditorApp::draw_ui() {
  ImGuiWindowFlags dock_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  dock_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, dock_flags);
  ImGui::PopStyleVar(3);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        host_.request_close();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  const ImGuiID dockspace_id = ImGui::GetID("RatDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::Begin("Hierarchy");
  ImGui::Text("Mode: %s  (F2)", app_mode_name(app_mode_));
  ImGui::Text("Map: %s", session_.state().map_id().c_str());
  ImGui::Text("Player: (%.2f, %.2f, %.2f)", session_.player().x, session_.player().y,
              session_.player().z);
  ImGui::Text("Events: %zu", session_.events().map().events.size());
  ImGui::Text("Parallel: %d", session_.events().active_parallel_count());
  ImGui::End();

  ImGui::Begin("Inspector");
  ImGui::Text("App mode: %s", app_mode_name(app_mode_));
  if (app_mode_ == AppMode::Edit) {
    ImGui::TextUnformatted("EDIT: player + events paused. Edit elevation/blockers/events below.");
  } else {
    ImGui::TextUnformatted("Quest: ask foreman (cyan) for a rusty cog,");
    ImGui::TextUnformatted("loot scrap east of crates, return.");
  }
  ImGui::TextUnformatted(
      "WASD move | Space jump | E interact | Esc pause | C camera | F2 Play/Edit | F3 snapshot | F5 hot-apply");
  if (app_mode_ == AppMode::Play) {
    ImGui::TextUnformatted("Escape pause | Save/Load slot saves/slot1.ratsave");
    ImGui::TextUnformatted("I inventory");
  }
  if (app_mode_ == AppMode::Edit) {
    ImGui::TextUnformatted("Ctrl+Z undo | Ctrl+Y / Ctrl+Shift+Z redo");
  }
  if (ImGui::Button(app_mode_ == AppMode::Play ? "Enter Edit (F2)" : "Enter Play (F2)")) {
    set_app_mode(toggle_app_mode(app_mode_));
  }
  if (!map_path_.empty()) {
    ImGui::TextWrapped("Map file: %s", map_path_.c_str());
  }
  if (app_mode_ == AppMode::Edit && !map_path_.empty() &&
      ImGui::Button("Save current map JSON")) {
    save_map_path(map_path_);
  }
  if (app_mode_ == AppMode::Edit && ImGui::Button("Apply edited map")) {
    apply_edited_map(true);
  }
  if (!map_path_.empty() && ImGui::Button("Load map JSON (F5, keep pos)")) {
    hot_apply_map_path(map_path_, true);
  }
  if (!map_path_.empty() && ImGui::Button("Reload map (reset player)")) {
    if (hot_apply_map_path(map_path_, false)) {
      session_.player().x = -1.5f;
      session_.player().z = 1.5f;
      snap_player_to_ground_clear_jump();
    }
  }
  if (!last_apply_error_.empty()) {
    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Map I/O error: %s",
                       last_apply_error_.c_str());
  }
  if (!last_serialize_status_.empty()) {
    ImGui::TextWrapped("%s", last_serialize_status_.c_str());
  }
  if (!last_save_status_.empty()) {
    ImGui::TextWrapped("%s", last_save_status_.c_str());
  }
  if (app_mode_ == AppMode::Edit) {
    ImGui::Separator();
    ImGui::TextUnformatted("Edit submode");
    auto apply_submode = [&](EditSubmode next) {
      if (edit_submode_ == next) {
        return;
      }
      edit_submode_ = next;
      viewport_tool_ = ViewportTool::Select;
      if (next != EditSubmode::Events) {
        event_graph_open_ = false;
      }
      if (next == EditSubmode::Terrain || next == EditSubmode::Objects) {
        document_.select_event(-1);
      }
      if (next == EditSubmode::Terrain || next == EditSubmode::Events) {
        document_.select_blocker(-1);
      }
      if (!viewport_tool_allowed(edit_submode_, viewport_tool_)) {
        viewport_tool_ = ViewportTool::Select;
      }
    };
    if (ImGui::RadioButton("Terrain", edit_submode_ == EditSubmode::Terrain)) {
      apply_submode(EditSubmode::Terrain);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Objects", edit_submode_ == EditSubmode::Objects)) {
      apply_submode(EditSubmode::Objects);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Events", edit_submode_ == EditSubmode::Events)) {
      apply_submode(EditSubmode::Events);
    }
    if (!viewport_tool_allowed(edit_submode_, viewport_tool_)) {
      viewport_tool_ = ViewportTool::Select;
    }
    ImGui::TextUnformatted("Mouse map tool");
    bool first_tool = true;
    auto tool_radio = [&](const char* label, ViewportTool tool) {
      if (!viewport_tool_allowed(edit_submode_, tool)) {
        return;
      }
      if (!first_tool) {
        ImGui::SameLine();
      }
      first_tool = false;
      if (ImGui::RadioButton(label, viewport_tool_ == tool)) {
        viewport_tool_ = tool;
      }
    };
    tool_radio("Select", ViewportTool::Select);
    tool_radio("Place blocker", ViewportTool::PlaceBlocker);
    tool_radio("Place event", ViewportTool::PlaceEvent);
    tool_radio("Place cube", ViewportTool::PlaceCube);
    tool_radio("Fence", ViewportTool::PlaceFence);
    tool_radio("Floor slab", ViewportTool::PlaceSlab);
    tool_radio("Ladder", ViewportTool::PlaceLadder);
    if (edit_submode_ == EditSubmode::Terrain) {
      ImGui::TextUnformatted("Fence preset");
      ImGui::SameLine();
      ImGui::RadioButton("Mini 0.45", &fence_preset_index_, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Full 1.6", &fence_preset_index_, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Remove", &fence_preset_index_, 2);
      draw_terrain_panel(document_, terrain_panel_);
    } else if (edit_submode_ == EditSubmode::Objects) {
      float sampled_ground_y = 0.0f;
      if (document_.selected_blocker() >= 0 &&
          document_.selected_blocker() <
              static_cast<int>(document_.visible_data().blockers.size())) {
        const BlockerDef& selected =
            document_.visible_data().blockers[static_cast<std::size_t>(document_.selected_blocker())];
        const float center_x = 0.5f * (selected.bounds.min_x + selected.bounds.max_x);
        const float center_z = 0.5f * (selected.bounds.min_z + selected.bounds.max_z);
        if (session_.surface_query() == nullptr) {
          session_.rebuild_surface();
        }
        sampled_ground_y = session_.surface_query() == nullptr
                               ? 0.0f
                               : session_.surface_query()->sample(center_x, center_z).y;
      }
      draw_blocker_panel(document_, blocker_panel_, sampled_ground_y, last_apply_error_);
      draw_ladder_panel(document_, terrain_panel_);
    } else {
      const char* why_not = "";
      if (document_.selected_event() >= 0 &&
          document_.selected_event() < static_cast<int>(document_.visible_data().events.size())) {
        why_not = event_why_not_name(session_.events().why_not_fired(
            document_.visible_data().events[static_cast<std::size_t>(document_.selected_event())].id,
            session_.state(), session_.player(), false));
      }
      draw_event_panel(document_, event_panel_, why_not, &event_graph_open_);
    }
  }
  {
    int mode = 0;
    switch (engine_->greybox().camera_mode()) {
      case CameraMode::TopDown:
        mode = 0;
        break;
      case CameraMode::Tilt45:
        mode = 1;
        break;
      case CameraMode::ThreeQuarter:
        mode = 2;
        break;
    }
    if (ImGui::Combo("Camera", &mode, "Top-down\0Tilt 45\0Ortho 3/4\0")) {
      const CameraMode selected = mode == 0   ? CameraMode::TopDown
                                  : mode == 1 ? CameraMode::Tilt45
                                              : CameraMode::ThreeQuarter;
      engine_->greybox().set_camera_mode(selected);
    }
  }
  if (ImGui::Button("Snap player to grid")) {
    const float tile_size =
        document_.visible_data().tile_size > 0.0f ? document_.visible_data().tile_size : 1.0f;
    const auto snapped =
        snap_to_grid(session_.player().x, session_.player().y, session_.player().z, tile_size);
    session_.player().x = snapped.x;
    session_.player().z = snapped.z;
    snap_player_to_ground_clear_jump();
  }
  ImGui::Separator();
  ImGui::Text("Quest accepted (sw1): %s", session_.state().get_switch(1) ? "ON" : "OFF");
  ImGui::Text("Quest done (sw2): %s", session_.state().get_switch(2) ? "ON" : "OFF");
  ImGui::Text("Scrap looted (self A): %s",
              session_.state().get_self_switch("scrap_pile", 'A') ? "ON" : "OFF");
  ImGui::Text("Intro var0: %d", session_.state().get_variable(0));
  ImGui::Text("rusty_cog: %d", session_.state().item_quantity("rusty_cog"));
  ImGui::End();

  if (event_context_open_) {
    ImGui::OpenPopup("event_context");
    event_context_open_ = false;
  }
  if (ImGui::BeginPopup("event_context")) {
    const auto& events = document_.visible_data().events;
    const bool on_event = event_context_index_ >= 0 &&
                          event_context_index_ < static_cast<int>(events.size());
    if (ImGui::MenuItem("Create")) {
      TileCoord tile = event_context_tile_;
      if (on_event && events[static_cast<std::size_t>(event_context_index_)].tile.has_value()) {
        tile = *events[static_cast<std::size_t>(event_context_index_)].tile;
      }
      const std::string id = "stub_" + std::to_string(event_panel_.next_stub_event++);
      (void)document_.execute(make_place_event_command(make_stub_event(id, tile.x, tile.z)));
      document_.select_event(static_cast<int>(document_.data().events.size()) - 1);
    }
    if (on_event) {
      if (ImGui::MenuItem("Edit")) {
        document_.select_event(event_context_index_);
        event_graph_open_ = true;
      }
      if (ImGui::MenuItem("Copy")) {
        (void)document_.execute(make_duplicate_event_command(
            document_.visible_data(), static_cast<std::size_t>(event_context_index_)));
        document_.select_event(static_cast<int>(document_.data().events.size()) - 1);
      }
      if (ImGui::MenuItem("Delete")) {
        (void)document_.execute(
            make_delete_event_command(static_cast<std::size_t>(event_context_index_)));
      }
    }
    ImGui::EndPopup();
  }

  if (app_mode_ == AppMode::Edit && edit_submode_ == EditSubmode::Events && event_graph_open_) {
    const int selected = document_.selected_event();
    const auto& events = document_.visible_data().events;
    if (selected >= 0 && selected < static_cast<int>(events.size())) {
      draw_event_graph_window(document_, event_panel_, &event_graph_open_);
    }
  }

  if (app_mode_ == AppMode::Play && play_paused_) {
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + 24.0f, viewport->WorkPos.y + 80.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowBgAlpha(0.94f);
    ImGui::Begin("Pause", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextUnformatted("Paused");
    ImGui::TextWrapped("Slot: %s", kPlaySaveSlotPath);
    if (ImGui::Button("Save")) {
      save_play_slot();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
      load_play_slot();
    }
    ImGui::TextUnformatted("Escape pause");
    if (!last_save_status_.empty()) {
      ImGui::TextWrapped("%s", last_save_status_.c_str());
    }
    ImGui::End();
  }

  if (app_mode_ == AppMode::Play && inventory_open_ && !play_paused_) {
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + 24.0f, viewport->WorkPos.y + 80.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowBgAlpha(0.94f);
    ImGui::Begin("Inventory", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextUnformatted("Inventory");
    ImGui::Separator();
    for (const std::string& row : format_inventory_rows(session_.state().inventory())) {
      ImGui::TextUnformatted(row.c_str());
    }
    ImGui::TextUnformatted("I close");
    ImGui::End();
  }

  if (event_runtime_enabled(app_mode_) &&
      session_.events().has_action_prompt(session_.player(), session_.state()) &&
      !session_.events().active_message().has_value()) {
    const ImVec2 prompt_pos(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f - 70.0f,
                            viewport->WorkPos.y + viewport->WorkSize.y * 0.55f);
    ImGui::SetNextWindowPos(prompt_pos);
    ImGui::SetNextWindowBgAlpha(0.65f);
    ImGui::Begin("InteractPrompt", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::TextUnformatted("[E] Interact");
    ImGui::End();
  }

  if (event_runtime_enabled(app_mode_) && session_.events().active_message().has_value()) {
    const float dialog_w = viewport->WorkSize.x - 160.0f;
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + 80.0f, viewport->WorkPos.y + viewport->WorkSize.y - 170.0f));
    ImGui::SetNextWindowSize(ImVec2(dialog_w, 130.0f));
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::Begin("Dialog", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavInputs |
                     ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::TextUnformatted("Dialog");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("%s", session_.events().active_message()->c_str());
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 36.0f);
    if (ImGui::Button("Continue (E)")) {
      session_.events().acknowledge_message();
    }
    ImGui::End();
  }

  ImGui::End();

  if (app_mode_ == AppMode::Edit && edit_submode_ == EditSubmode::Events && engine_ != nullptr) {
    const MapData& map = document_.visible_data();
    const std::vector<Vec3> markers = event_markers_from_map(map);
    const std::uint32_t fb_w = static_cast<std::uint32_t>(width_ > 0 ? width_ : 1);
    const std::uint32_t fb_h = static_cast<std::uint32_t>(height_ > 0 ? height_ : 1);
    const OrthoCamera& camera = engine_->greybox().camera();
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    std::size_t marker_i = 0;
    for (const EventDef& event : map.events) {
      if (!event.tile.has_value() && !event.volume.has_value()) {
        continue;
      }
      if (marker_i >= markers.size()) {
        break;
      }
      Vec3 stem_top = markers[marker_i++];
      stem_top.y += kEventMarkerStemHeight;
      const auto pixel = project_world_to_pixels(camera, stem_top, fb_w, fb_h);
      if (!pixel.has_value()) {
        continue;
      }
      const ImVec2 text_size = ImGui::CalcTextSize(event.id.c_str());
      draw_list->AddText(ImVec2(pixel->x - text_size.x * 0.5f, pixel->y - text_size.y),
                         IM_COL32(255, 255, 255, 255), event.id.c_str());
    }
  }
}

}  // namespace rat
