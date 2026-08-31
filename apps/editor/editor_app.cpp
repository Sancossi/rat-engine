#include "editor_app.hpp"

#include "imgui_bgfx.hpp"

#include <rat/blocker_edit.hpp>
#include <rat/engine.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_inspect.hpp>
#include <rat/hot_apply.hpp>
#include <rat/map_loader.hpp>
#include <rat/surface_query.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <string>
#include <vector>

namespace rat {

EditorApp::~EditorApp() {
  shutdown();
}

void EditorApp::sync_blockers_to_runtime() {
  if (engine_ == nullptr) {
    return;
  }
  rebuild_surface_query_cache();
  engine_->set_terrain_map(events_.map());
  engine_->set_blockers(events_.map().blockers);
  engine_->greybox().set_selected_blocker(app_mode_ == AppMode::Edit ? selected_blocker_ : -1);
}

void EditorApp::sync_events_to_runtime() {
  if (engine_ == nullptr) {
    return;
  }
  rebuild_surface_query_cache();
  engine_->set_terrain_map(events_.map());
  engine_->set_event_markers(event_markers_from_map(events_.map()));
  const int marker =
      app_mode_ == AppMode::Edit ? event_marker_index(events_.map(), selected_event_) : -1;
  engine_->greybox().set_selected_event_marker(marker);
}

void EditorApp::draw_blocker_edit_ui() {
  ImGui::Separator();
  ImGui::TextUnformatted("Blockers (Edit)");
  const float tile = events_.map().tile_size > 0.0f ? events_.map().tile_size : 1.0f;
  auto blockers = events_.map().blockers;

  if (ImGui::Button("Add blocker")) {
    blockers.push_back(BlockerDef{.bounds = Aabb2{0.0f, 0.0f, tile, tile}});
    selected_blocker_ = static_cast<int>(blockers.size()) - 1;
    events_.set_blockers(blockers);
    sync_blockers_to_runtime();
    blockers = events_.map().blockers;
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete selected") && selected_blocker_ >= 0 &&
      selected_blocker_ < static_cast<int>(blockers.size())) {
    blockers.erase(blockers.begin() + selected_blocker_);
    if (blockers.empty()) {
      selected_blocker_ = -1;
    } else if (selected_blocker_ >= static_cast<int>(blockers.size())) {
      selected_blocker_ = static_cast<int>(blockers.size()) - 1;
    }
    events_.set_blockers(blockers);
    sync_blockers_to_runtime();
    blockers = events_.map().blockers;
  }

  if (ImGui::BeginListBox("##blockers", ImVec2(-1.0f, 100.0f))) {
    for (int i = 0; i < static_cast<int>(blockers.size()); ++i) {
      const auto& b = blockers[static_cast<std::size_t>(i)].bounds;
      char label[128];
      std::snprintf(label, sizeof(label), "%d: (%.0f,%.0f)-(%.0f,%.0f)", i, b.min_x, b.min_z,
                    b.max_x, b.max_z);
      if (ImGui::Selectable(label, selected_blocker_ == i)) {
        selected_blocker_ = i;
        sync_blockers_to_runtime();
      }
    }
    ImGui::EndListBox();
  }

  if (selected_blocker_ >= 0 && selected_blocker_ < static_cast<int>(blockers.size())) {
    BlockerDef& blocker = blockers[static_cast<std::size_t>(selected_blocker_)];
    Aabb2& box = blocker.bounds;
    ImGui::Text("Selected %d", selected_blocker_);
    auto apply_box = [&](Aabb2 next) {
      blockers[static_cast<std::size_t>(selected_blocker_)].bounds = next;
      events_.set_blockers(blockers);
      sync_blockers_to_runtime();
      blockers = events_.map().blockers;
    };

    if (ImGui::Button("-X")) {
      apply_box(translate_aabb_on_grid(box, -1, 0, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("+X")) {
      apply_box(translate_aabb_on_grid(box, 1, 0, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("-Z")) {
      apply_box(translate_aabb_on_grid(box, 0, -1, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("+Z")) {
      apply_box(translate_aabb_on_grid(box, 0, 1, tile));
    }

    if (ImGui::Button("Grow +X")) {
      apply_box(resize_aabb_on_grid(box, AabbEdge::MaxX, 1, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("Shrink +X")) {
      apply_box(resize_aabb_on_grid(box, AabbEdge::MaxX, -1, tile));
    }
    if (ImGui::Button("Grow +Z")) {
      apply_box(resize_aabb_on_grid(box, AabbEdge::MaxZ, 1, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("Shrink +Z")) {
      apply_box(resize_aabb_on_grid(box, AabbEdge::MaxZ, -1, tile));
    }
    if (ImGui::Button("Snap to grid")) {
      apply_box(snap_aabb_to_grid(box, tile));
    }
  }

}

void EditorApp::draw_event_edit_ui() {
  ImGui::Separator();
  ImGui::TextUnformatted("Events (Edit)");
  const float tile = events_.map().tile_size > 0.0f ? events_.map().tile_size : 1.0f;
  auto event_list = events_.map().events;

  if (ImGui::Button("Add stub event")) {
    const std::string id = "stub_" + std::to_string(next_stub_event_++);
    event_list.push_back(make_stub_event(id, 0, 0));
    selected_event_ = static_cast<int>(event_list.size()) - 1;
    events_.set_events(event_list);
    sync_events_to_runtime();
    event_list = events_.map().events;
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete event") && selected_event_ >= 0 &&
      selected_event_ < static_cast<int>(event_list.size())) {
    event_list.erase(event_list.begin() + selected_event_);
    if (event_list.empty()) {
      selected_event_ = -1;
    } else if (selected_event_ >= static_cast<int>(event_list.size())) {
      selected_event_ = static_cast<int>(event_list.size()) - 1;
    }
    events_.set_events(event_list);
    sync_events_to_runtime();
    event_list = events_.map().events;
  }

  if (ImGui::BeginListBox("##events", ImVec2(-1.0f, 120.0f))) {
    for (int i = 0; i < static_cast<int>(event_list.size()); ++i) {
      const auto& ev = event_list[static_cast<std::size_t>(i)];
      char label[160];
      if (ev.tile.has_value()) {
        std::snprintf(label, sizeof(label), "%s  tile(%d,%d)", ev.id.c_str(), ev.tile->x,
                      ev.tile->z);
      } else if (ev.volume.has_value()) {
        std::snprintf(label, sizeof(label), "%s  vol", ev.id.c_str());
      } else {
        std::snprintf(label, sizeof(label), "%s  (no place)", ev.id.c_str());
      }
      if (ImGui::Selectable(label, selected_event_ == i)) {
        selected_event_ = i;
        selected_page_ = 0;
        sync_events_to_runtime();
      }
    }
    ImGui::EndListBox();
  }

  if (selected_event_ >= 0 && selected_event_ < static_cast<int>(event_list.size())) {
    auto apply_event = [&](EventDef next) {
      event_list[static_cast<std::size_t>(selected_event_)] = std::move(next);
      events_.set_events(event_list);
      sync_events_to_runtime();
      event_list = events_.map().events;
    };

    EventDef event = event_list[static_cast<std::size_t>(selected_event_)];
    ImGui::Text("id: %s", event.id.c_str());
    ImGui::Text("pages: %zu", event.pages.size());

    if (!event.tile.has_value() && !event.volume.has_value()) {
      if (ImGui::Button("Place on tile (0,0)")) {
        event.tile = TileCoord{0, 0};
        apply_event(std::move(event));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
    } else {
      if (ImGui::Button("Ev -X")) {
        translate_event_on_grid(event, -1, 0, tile);
        apply_event(std::move(event));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::SameLine();
      if (ImGui::Button("Ev +X")) {
        translate_event_on_grid(event, 1, 0, tile);
        apply_event(std::move(event));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::SameLine();
      if (ImGui::Button("Ev -Z")) {
        translate_event_on_grid(event, 0, -1, tile);
        apply_event(std::move(event));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::SameLine();
      if (ImGui::Button("Ev +Z")) {
        translate_event_on_grid(event, 0, 1, tile);
        apply_event(std::move(event));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Page inspector");
    if (event.pages.empty()) {
      ImGui::TextUnformatted("(no pages)");
    } else {
      if (selected_page_ < 0 || selected_page_ >= static_cast<int>(event.pages.size())) {
        selected_page_ = 0;
      }
      if (ImGui::BeginListBox("##pages", ImVec2(-1.0f, 80.0f))) {
        for (int p = 0; p < static_cast<int>(event.pages.size()); ++p) {
          const EventPage& page = event.pages[static_cast<std::size_t>(p)];
          char label[192];
          std::snprintf(label, sizeof(label), "%d: %s | %s", p, trigger_kind_name(page.trigger),
                        summarize_page_conditions(page).c_str());
          if (ImGui::Selectable(label, selected_page_ == p)) {
            selected_page_ = p;
          }
        }
        ImGui::EndListBox();
      }

      EventPage& page = event.pages[static_cast<std::size_t>(selected_page_)];
      int trigger = static_cast<int>(page.trigger);
      if (ImGui::Combo("Trigger", &trigger,
                       "action\0player_touch\0event_touch\0autorun\0parallel\0")) {
        page.trigger = static_cast<TriggerKind>(trigger);
        apply_event(event);
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::TextWrapped("Conditions: %s", summarize_page_conditions(page).c_str());

      int sw_i = -1;
      for (int i = 0; i < static_cast<int>(page.conditions.size()); ++i) {
        if (page.conditions[static_cast<std::size_t>(i)].type == ConditionType::Switch) {
          sw_i = i;
          break;
        }
      }
      if (sw_i < 0) {
        if (ImGui::Button("Add enable Switch")) {
          (void)ensure_page_enable_switch(page, 1, true);
          apply_event(event);
          event = event_list[static_cast<std::size_t>(selected_event_)];
        }
      } else {
        Condition& sw = event.pages[static_cast<std::size_t>(selected_page_)]
                            .conditions[static_cast<std::size_t>(sw_i)];
        int switch_id = static_cast<int>(sw.id);
        bool on = sw.bool_value;
        bool dirty = false;
        if (ImGui::InputInt("Enable SW id", &switch_id)) {
          if (switch_id < 0) {
            switch_id = 0;
          }
          sw.id = static_cast<std::uint32_t>(switch_id);
          dirty = true;
        }
        if (ImGui::Checkbox("Enable SW ON", &on)) {
          sw.bool_value = on;
          dirty = true;
        }
        if (dirty) {
          apply_event(event);
          event = event_list[static_cast<std::size_t>(selected_event_)];
        }
      }

      EventPage& page_now = event.pages[static_cast<std::size_t>(selected_page_)];
      int text_i = find_first_show_text(page_now);
      if (text_i < 0) {
        if (ImGui::Button("Add Show Text")) {
          Command cmd;
          cmd.op = CommandOp::ShowText;
          cmd.text = "New text";
          page_now.commands.insert(page_now.commands.begin(), std::move(cmd));
          apply_event(event);
          event = event_list[static_cast<std::size_t>(selected_event_)];
        }
      } else {
        Command& cmd = event.pages[static_cast<std::size_t>(selected_page_)]
                           .commands[static_cast<std::size_t>(text_i)];
        char buf[512];
        std::snprintf(buf, sizeof(buf), "%s", cmd.text.c_str());
        if (ImGui::InputTextMultiline("Show Text", buf, sizeof(buf), ImVec2(-1.0f, 60.0f))) {
          cmd.text = buf;
          apply_event(event);
        }
      }
    }
  }
}

bool EditorApp::hot_apply_map_path(const std::string& path, bool preserve_player) {
  if (engine_ == nullptr) {
    last_apply_error_ = "engine not ready";
    return false;
  }

  std::vector<BlockerDef> blockers;
  std::vector<Vec3> markers;
  HotApplyTargets targets{events_, game_state_, player_, blockers, markers, &surface_query_cache_,
                          &jump_state_};
  HotApplyOptions options;
  options.preserve_player_position = preserve_player;

  const HotApplyResult result = hot_apply_map_from_file(path, targets, options);
  if (!result.ok) {
    last_apply_error_ = result.error;
    std::fprintf(stderr, "hot-apply failed (%s): %s\n", path.c_str(), result.error.c_str());
    return false;
  }

  last_apply_error_.clear();
  map_path_ = path;
  selected_blocker_ = blockers.empty() ? -1 : 0;
  selected_event_ = events_.map().events.empty() ? -1 : 0;
  selected_page_ = 0;
  engine_->set_terrain_map(events_.map());
  engine_->set_blockers(std::move(blockers));
  engine_->set_event_markers(std::move(markers));
  engine_->set_player(player_);
  engine_->greybox().set_selected_blocker(app_mode_ == AppMode::Edit ? selected_blocker_ : -1);
  engine_->greybox().set_selected_event_marker(
      app_mode_ == AppMode::Edit ? event_marker_index(events_.map(), selected_event_) : -1);
  jump_state_ = make_grounded_jump_state();
  jump_state_.coyote_time_left = jump_tuning_.coyote_seconds;
  jump_state_.jump_buffer_left = 0.0f;
  interact_was_down_ = window_ != nullptr && glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
  clear_buffered_press(interact_press_buffer_);
  jump_was_down_ = window_ != nullptr && glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
  jump_press_pending_ = false;
  fixed_accumulator_ = 0.0f;
  snap_player_to_ground_clear_jump();
  return true;
}

bool EditorApp::save_map_path(const std::string& path) {
  if (path.empty()) {
    last_apply_error_ = "map path is empty";
    return false;
  }
  const MapFileResult result = save_map_to_file(events_.map(), path);
  if (!result.ok) {
    last_apply_error_ = result.error;
    last_serialize_status_.clear();
    std::fprintf(stderr, "map save failed (%s): %s\n", path.c_str(), result.error.c_str());
    return false;
  }
  last_apply_error_.clear();
  last_serialize_status_ = "Saved: " + path;
  return true;
}

bool EditorApp::init() {
  if (!glfwInit()) {
    std::fprintf(stderr, "glfwInit failed\n");
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(width_, height_, "rat-editor", nullptr, nullptr);
  if (window_ == nullptr) {
    std::fprintf(stderr, "glfwCreateWindow failed\n");
    glfwTerminate();
    return false;
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
  glfwGetFramebufferSize(window_, &width_, &height_);

  engine_ = new Engine();

  RendererConfig config;
  config.window.nwh = glfwGetWin32Window(window_);
  config.width = static_cast<std::uint32_t>(width_ > 0 ? width_ : 1);
  config.height = static_cast<std::uint32_t>(height_ > 0 ? height_ : 1);
  config.vsync = true;

  if (!engine_->init(config)) {
    std::fprintf(stderr, "Engine::init failed\n");
    shutdown();
    return false;
  }
  engine_->set_debug_banner("rat-engine");

#ifndef RAT_DATA_DIR
#error RAT_DATA_DIR must be defined
#endif
  const std::string map_path = std::string(RAT_DATA_DIR) + "/maps/grey_yard.json";
  if (!hot_apply_map_path(map_path, false)) {
    std::fprintf(stderr, "Failed to load map %s: %s\n", map_path.c_str(),
                 last_apply_error_.c_str());
    shutdown();
    return false;
  }

  // Spawn near the foreman for the sample quest path.
  player_.x = -1.5f;
  player_.z = 1.5f;
  snap_player_to_ground_clear_jump();

  refresh_mode_banner();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOther(window_, true)) {
    std::fprintf(stderr, "ImGui_ImplGlfw_InitForOther failed\n");
    shutdown();
    return false;
  }

  if (!imgui_bgfx::init(255)) {
    std::fprintf(stderr, "imgui_bgfx::init failed\n");
    shutdown();
    return false;
  }

  last_time_ = glfwGetTime();
  running_ = true;
  return true;
}

int EditorApp::run() {
  if (!running_) {
    return 1;
  }

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    const double now = glfwGetTime();
    float dt = static_cast<float>(now - last_time_);
    last_time_ = now;
    if (dt < 0.0f) {
      dt = 0.0f;
    }
    if (dt > 0.1f) {
      dt = 0.1f;
    }
    update_simulation(dt);

    ImGui_ImplGlfw_NewFrame();
    imgui_bgfx::begin_frame(width_, height_);
    draw_ui();

    engine_->begin_frame();
    imgui_bgfx::end_frame();
    engine_->end_frame();
  }

  shutdown();
  return 0;
}

void EditorApp::shutdown() {
  if (!running_ && window_ == nullptr && engine_ == nullptr) {
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

  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  glfwTerminate();
}

void EditorApp::on_framebuffer_resize(int width, int height) {
  width_ = width > 0 ? width : 1;
  height_ = height > 0 ? height : 1;
  if (engine_ != nullptr && engine_->is_initialized()) {
    engine_->resize(static_cast<std::uint32_t>(width_),
                    static_cast<std::uint32_t>(height_));
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
  const AppMode prev_mode = app_mode_;
  app_mode_ = next_mode;
  refresh_mode_banner();
  sync_blockers_to_runtime();
  sync_events_to_runtime();
  if (prev_mode != AppMode::Edit && app_mode_ == AppMode::Edit) {
    clear_buffered_press(interact_press_buffer_);
  }
}

void EditorApp::rebuild_surface_query_cache() {
  surface_query_cache_ = std::make_unique<SurfaceQuery>(events_.map());
}

void EditorApp::snap_player_to_ground_clear_jump() {
  if (surface_query_cache_ == nullptr) {
    rebuild_surface_query_cache();
  }
  const SurfaceSample sample = surface_query_cache_->sample(player_.x, player_.z);
  jump_state_ = make_grounded_jump_state();
  jump_state_.grounded = true;
  jump_state_.jump_offset = 0.0f;
  jump_state_.vertical_speed = 0.0f;
  jump_state_.coyote_time_left = jump_tuning_.coyote_seconds;
  jump_state_.jump_buffer_left = 0.0f;
  player_.y = sample.y;
  game_state_.set_player_position(player_.x, player_.y, player_.z);
  if (engine_ != nullptr) {
    engine_->set_player(player_);
  }
}

void EditorApp::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  auto* self = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (self != nullptr) {
    self->on_framebuffer_resize(width, height);
  }
}

void EditorApp::update_simulation(float dt) {
  if (engine_ == nullptr || window_ == nullptr) {
    return;
  }

  const ImGuiIO& io = ImGui::GetIO();
  constexpr float kFixedStep = 1.0f / 120.0f;

  // F2 toggles Play <-> Edit (edge). Allowed even if ImGui wants keyboard
  // except when typing into an active text field would be ideal later; for now
  // skip only when a dialog message is open so Space/E ack stays clean.
  const bool mode_down = glfwGetKey(window_, GLFW_KEY_F2) == GLFW_PRESS;
  if (mode_down && !mode_toggle_was_down_ && !events_.active_message().has_value()) {
    set_app_mode(toggle_app_mode(app_mode_));
  }
  mode_toggle_was_down_ = mode_down;

  // F5 hot-applies current map JSON (preserves player). Works in Play and Edit.
  const bool hot_apply_down = glfwGetKey(window_, GLFW_KEY_F5) == GLFW_PRESS;
  if (hot_apply_down && !hot_apply_was_down_ && !events_.active_message().has_value() &&
      !map_path_.empty()) {
    clear_buffered_press(interact_press_buffer_);
    hot_apply_map_path(map_path_, true);
  }
  hot_apply_was_down_ = hot_apply_down;

  const bool interact_down = glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS;
  const bool interact_edge = interact_down && !interact_was_down_;
  interact_was_down_ = interact_down;
  push_buffered_press_if_allowed(interact_press_buffer_, interact_edge, event_runtime_enabled(app_mode_),
                                 io.WantCaptureKeyboard, 0.1f);

  const bool jump_down = glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
  const bool jump_edge = jump_down && !jump_was_down_;
  jump_was_down_ = jump_down;
  if (jump_edge) {
    jump_press_pending_ = true;
  }

  bool consumed_for_dialog = false;
  if (event_runtime_enabled(app_mode_) && events_.active_message().has_value() && !io.WantCaptureKeyboard &&
      consume_buffered_press(interact_press_buffer_)) {
    events_.acknowledge_message();
    consumed_for_dialog = true;
  }

  // C cycles TopDown -> Tilt45 -> 3/4 (edge, ignore while typing in ImGui).
  const bool camera_down = glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS;
  if (camera_down && !camera_toggle_was_down_ && !io.WantCaptureKeyboard &&
      !events_.active_message().has_value()) {
    engine_->greybox().set_camera_mode(next_camera_mode(engine_->greybox().camera_mode()));
  }
  camera_toggle_was_down_ = camera_down;

  fixed_accumulator_ += std::max(0.0f, dt);
  fixed_accumulator_ = std::min(fixed_accumulator_, 0.2f);
  while (fixed_accumulator_ >= kFixedStep) {
    fixed_accumulator_ -= kFixedStep;

    MoveInput move_input;
    const bool allow_player_input =
        player_control_enabled(app_mode_) && !io.WantCaptureKeyboard && !events_.player_input_blocked();
    if (allow_player_input) {
      float screen_x = 0.0f;
      float screen_z = 0.0f;
      if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS ||
          glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
        screen_z += 1.0f;
      }
      if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS ||
          glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
        screen_z -= 1.0f;
      }
      if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS ||
          glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS) {
        screen_x -= 1.0f;
      }
      if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS ||
          glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        screen_x += 1.0f;
      }
      move_input = world_aligned_move(screen_x, screen_z);
    } else {
      jump_state_.jump_buffer_left = 0.0f;
      jump_press_pending_ = false;
    }

    if (player_control_enabled(app_mode_)) {
      PlayerFrameInput frame_input;
      frame_input.move = move_input;
      frame_input.jump_pressed = allow_player_input && jump_press_pending_;
      frame_input.jump_held = allow_player_input && jump_down;

      if (surface_query_cache_ == nullptr) {
        rebuild_surface_query_cache();
      }
      const PlayerFrameResult frame = integrate_player_frame_surface(
          player_, jump_state_, frame_input, kFixedStep, engine_->blockers(), *surface_query_cache_,
          jump_tuning_);
      player_ = frame.body;
      jump_state_ = frame.jump;
      game_state_.set_player_position(player_.x, player_.y, player_.z);
      engine_->set_player(player_);
      if (frame_input.jump_pressed) {
        jump_press_pending_ = false;
      }
    } else {
      jump_state_ = make_grounded_jump_state();
      jump_state_.coyote_time_left = jump_tuning_.coyote_seconds;
      jump_state_.jump_buffer_left = 0.0f;
      jump_press_pending_ = false;
      snap_player_to_ground_clear_jump();
    }

    if (event_runtime_enabled(app_mode_)) {
      // Same key edge that closes dialog must not also fire Action triggers.
      const bool gameplay_interact = !consumed_for_dialog && !io.WantCaptureKeyboard &&
                                     !events_.player_input_blocked() &&
                                     consume_buffered_press(interact_press_buffer_);

      const std::string before_map_id = game_state_.map_id();
      const float before_x = game_state_.player_x();
      const float before_y = game_state_.player_y();
      const float before_z = game_state_.player_z();
      events_.update(game_state_, player_, gameplay_interact, kFixedStep);

      const bool transferred = game_state_.map_id() != before_map_id ||
                               game_state_.player_x() != before_x ||
                               game_state_.player_y() != before_y ||
                               game_state_.player_z() != before_z;
      player_.x = game_state_.player_x();
      player_.y = game_state_.player_y();
      player_.z = game_state_.player_z();
      engine_->set_player(player_);

      if (transferred) {
        jump_state_ = make_grounded_jump_state();
        jump_state_.coyote_time_left = jump_tuning_.coyote_seconds;
        jump_state_.jump_buffer_left = 0.0f;
        jump_was_down_ = jump_down;
        jump_press_pending_ = false;
      }
    }
    consume_then_tick_buffered_press(interact_press_buffer_, false, kFixedStep);
  }
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
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  const ImGuiID dockspace_id = ImGui::GetID("RatDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::Begin("Hierarchy");
  ImGui::Text("Mode: %s  (F2)", app_mode_name(app_mode_));
  ImGui::Text("Map: %s", game_state_.map_id().c_str());
  ImGui::Text("Player: (%.2f, %.2f, %.2f)", player_.x, player_.y, player_.z);
  ImGui::Text("Events: %zu", events_.map().events.size());
  ImGui::Text("Parallel: %d", events_.active_parallel_count());
  ImGui::End();

  ImGui::Begin("Inspector");
  ImGui::Text("App mode: %s", app_mode_name(app_mode_));
  if (app_mode_ == AppMode::Edit) {
    ImGui::TextUnformatted("EDIT: player + events paused. Edit blockers/events below.");
  } else {
    ImGui::TextUnformatted("Quest: ask foreman (cyan) for a rusty cog,");
    ImGui::TextUnformatted("loot scrap east of crates, return.");
  }
  ImGui::TextUnformatted("WASD move | Space jump | E interact | C camera | F2 Play/Edit | F5 hot-apply");
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
  if (!map_path_.empty() && ImGui::Button("Load map JSON (F5, keep pos)")) {
    hot_apply_map_path(map_path_, true);
  }
  if (!map_path_.empty() && ImGui::Button("Reload map (reset player)")) {
    if (hot_apply_map_path(map_path_, false)) {
      player_.x = -1.5f;
      player_.z = 1.5f;
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
  if (app_mode_ == AppMode::Edit) {
    draw_blocker_edit_ui();
    draw_event_edit_ui();
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
    const auto snapped = snap_to_grid(player_.x, player_.y, player_.z, 1.0f);
    player_.x = snapped.x;
    player_.y = snapped.y;
    player_.z = snapped.z;
    game_state_.set_player_position(player_.x, player_.y, player_.z);
    engine_->set_player(player_);
  }
  ImGui::Separator();
  ImGui::Text("Quest accepted (sw1): %s", game_state_.get_switch(1) ? "ON" : "OFF");
  ImGui::Text("Quest done (sw2): %s", game_state_.get_switch(2) ? "ON" : "OFF");
  ImGui::Text("Scrap looted (self A): %s",
              game_state_.get_self_switch("scrap_pile", 'A') ? "ON" : "OFF");
  ImGui::Text("Intro var0: %d", game_state_.get_variable(0));
  ImGui::Text("rusty_cog: %d", game_state_.item_quantity("rusty_cog"));
  ImGui::End();

  if (event_runtime_enabled(app_mode_) && events_.has_action_prompt(player_, game_state_) &&
      !events_.active_message().has_value()) {
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

  if (event_runtime_enabled(app_mode_) && events_.active_message().has_value()) {
    const float dialog_w = viewport->WorkSize.x - 160.0f;
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 80.0f,
                                   viewport->WorkPos.y + viewport->WorkSize.y - 170.0f));
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
    ImGui::TextWrapped("%s", events_.active_message()->c_str());
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 36.0f);
    if (ImGui::Button("Continue (E)")) {
      events_.acknowledge_message();
    }
    ImGui::End();
  }

  ImGui::End();
}

}  // namespace rat
