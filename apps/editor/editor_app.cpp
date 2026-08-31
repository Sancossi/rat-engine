#include "editor_app.hpp"

#include "imgui_bgfx.hpp"

#include <rat/blocker_edit.hpp>
#include <rat/debug_snapshot.hpp>
#include <rat/edit_history.hpp>
#include <rat/engine.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_inspect.hpp>
#include <rat/height_edit.hpp>
#include <rat/hot_apply.hpp>
#include <rat/input.hpp>
#include <rat/map_loader.hpp>
#include <rat/surface_query.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rat {

namespace {

InputButtons sample_editor_buttons(GLFWwindow* window) {
  InputButtons buttons;
  if (window == nullptr) {
    return buttons;
  }
  const auto down = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
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

}  // namespace

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

void EditorApp::apply_edited_map(MapData map, EditApplyResult mutation) {
  if (!mutation.applied) {
    return;
  }
  const int n_blockers = static_cast<int>(map.blockers.size());
  const int n_events = static_cast<int>(map.events.size());
  if (mutation.mutates_blockers) {
    if (n_blockers <= 0) {
      selected_blocker_ = -1;
    } else if (selected_blocker_ >= n_blockers) {
      selected_blocker_ = n_blockers - 1;
    }
    events_.set_blockers(std::move(map.blockers));
    sync_blockers_to_runtime();
  }
  if (mutation.mutates_events) {
    if (n_events <= 0) {
      selected_event_ = -1;
    } else if (selected_event_ >= n_events) {
      selected_event_ = n_events - 1;
    }
    events_.set_events(std::move(map.events));
    sync_events_to_runtime();
  }
}

void EditorApp::draw_blocker_edit_ui() {
  ImGui::Separator();
  ImGui::TextUnformatted("Blockers (Edit)");
  const float tile = events_.map().tile_size > 0.0f ? events_.map().tile_size : 1.0f;
  auto blockers = events_.map().blockers;
  auto run_history = [&](std::unique_ptr<EditCommand> command) {
    MapData map = events_.map();
    const EditApplyResult result = edit_history_.execute(map, std::move(command));
    apply_edited_map(std::move(map), result);
    blockers = events_.map().blockers;
  };

  if (ImGui::Button("Add blocker")) {
    BlockerDef blocker;
    blocker.bounds = Aabb2{0.0f, 0.0f, tile, tile};
    run_history(make_place_blocker_command(std::move(blocker)));
    selected_blocker_ = static_cast<int>(blockers.size()) - 1;
    sync_blockers_to_runtime();
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete selected") && selected_blocker_ >= 0 &&
      selected_blocker_ < static_cast<int>(blockers.size())) {
    run_history(make_delete_blocker_command(static_cast<std::size_t>(selected_blocker_)));
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
    auto submit_blockers = [&]() {
      events_.set_blockers(blockers);
      sync_blockers_to_runtime();
      blockers = events_.map().blockers;
    };
    auto selected_valid = [&]() {
      return selected_blocker_ >= 0 && selected_blocker_ < static_cast<int>(blockers.size());
    };

    ImGui::Text("Selected %d", selected_blocker_);
    auto apply_box = [&](Aabb2 next) {
      if (!selected_valid()) {
        return;
      }
      BlockerDef updated = blockers[static_cast<std::size_t>(selected_blocker_)];
      updated.bounds = next;
      blockers[static_cast<std::size_t>(selected_blocker_)] = updated;
      submit_blockers();
    };

    if (!selected_valid()) {
      return;
    }
    BlockerDef current = blockers[static_cast<std::size_t>(selected_blocker_)];
    const Aabb2 box = current.bounds;

    if (ImGui::Button("-X")) {
      run_history(make_move_blocker_command(static_cast<std::size_t>(selected_blocker_), -1, 0, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("+X")) {
      run_history(make_move_blocker_command(static_cast<std::size_t>(selected_blocker_), 1, 0, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("-Z")) {
      run_history(make_move_blocker_command(static_cast<std::size_t>(selected_blocker_), 0, -1, tile));
    }
    ImGui::SameLine();
    if (ImGui::Button("+Z")) {
      run_history(make_move_blocker_command(static_cast<std::size_t>(selected_blocker_), 0, 1, tile));
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

    if (!selected_valid()) {
      return;
    }
    current = blockers[static_cast<std::size_t>(selected_blocker_)];

    ImGui::Separator();
    bool jumpable = current.jumpable;
    if (ImGui::Checkbox("Jumpable vertical blocker", &jumpable)) {
      BlockerDef updated = current;
      if (jumpable) {
        const float center_x = 0.5f * (updated.bounds.min_x + updated.bounds.max_x);
        const float center_z = 0.5f * (updated.bounds.min_z + updated.bounds.max_z);
        if (surface_query_cache_ == nullptr) {
          rebuild_surface_query_cache();
        }
        const float sampled = surface_query_cache_ == nullptr
                                  ? 0.0f
                                  : surface_query_cache_->sample(center_x, center_z).y;
        const std::optional<float> base = updated.base_y.has_value() ? updated.base_y : sampled;
        const std::optional<float> top =
            updated.top_y.has_value() ? updated.top_y : (sampled + 0.9f);
        if (!set_blocker_vertical_range(updated, true, base, top)) {
          last_apply_error_ = "Invalid blocker vertical pair";
        } else {
          last_apply_error_.clear();
          blockers[static_cast<std::size_t>(selected_blocker_)] = updated;
          submit_blockers();
        }
      } else {
        (void)set_blocker_vertical_range(updated, false, std::nullopt, std::nullopt);
        last_apply_error_.clear();
        blockers[static_cast<std::size_t>(selected_blocker_)] = updated;
        submit_blockers();
      }
    }

    if (!selected_valid()) {
      return;
    }
    current = blockers[static_cast<std::size_t>(selected_blocker_)];

    if (current.jumpable) {
      float base_y = current.base_y.value_or(0.0f);
      float top_y = current.top_y.value_or(base_y + 0.9f);
      bool vertical_dirty = false;
      if (ImGui::InputFloat("Base Y", &base_y, 0.05f, 0.25f, "%.3f")) {
        vertical_dirty = true;
      }
      if (ImGui::InputFloat("Top Y", &top_y, 0.05f, 0.25f, "%.3f")) {
        vertical_dirty = true;
      }
      if (top_y < base_y) {
        top_y = base_y;
      }
      if (vertical_dirty) {
        BlockerDef updated = current;
        if (!set_blocker_vertical_range(updated, true, base_y, top_y)) {
          last_apply_error_ = "Invalid blocker vertical pair";
        } else {
          last_apply_error_.clear();
          blockers[static_cast<std::size_t>(selected_blocker_)] = updated;
          submit_blockers();
        }
      }
    } else {
      ImGui::TextUnformatted("Legacy full wall (no vertical pair).");
    }
  }

}

void EditorApp::draw_height_edit_ui() {
  ImGui::Separator();
  ImGui::TextUnformatted("Elevation (Edit)");
  if (ImGui::InputInt("Tile X", &height_tile_x_)) {
    // Keep immediate mode state only.
  }
  if (ImGui::InputInt("Tile Z", &height_tile_z_)) {
    // Keep immediate mode state only.
  }
  if (height_step_ <= 0.0f) {
    height_step_ = 0.25f;
  }
  ImGui::InputFloat("Step", &height_step_, 0.05f, 0.25f, "%.3f");
  if (height_step_ <= 0.0f) {
    height_step_ = 0.25f;
  }

  auto sync_elevation = [&]() {
    rebuild_surface_query_cache();
    if (engine_ != nullptr) {
      engine_->set_terrain_map(events_.map());
      engine_->set_event_markers(event_markers_from_map(events_.map()));
      engine_->set_blockers(events_.map().blockers);
    }
    snap_player_to_ground_clear_jump();
  };

  auto apply_result = [&](const HeightEditResult& result) {
    if (!result.ok) {
      last_apply_error_ = result.error;
      return false;
    }
    last_apply_error_.clear();
    sync_elevation();
    return true;
  };

  const HeightGetResult tile_height =
      get_tile_ground_y(events_.map().height_grid, height_tile_x_, height_tile_z_);
  const TileCoord ramp_tile{height_tile_x_, height_tile_z_};
  const int ramp_index = find_ramp_index_by_tile(events_.map().ramps, ramp_tile);
  if (!height_tile_sync_ready_ || last_height_tile_x_ != height_tile_x_ ||
      last_height_tile_z_ != height_tile_z_) {
    height_tile_sync_ready_ = true;
    last_height_tile_x_ = height_tile_x_;
    last_height_tile_z_ = height_tile_z_;
    if (tile_height.ok) {
      height_set_y_ = tile_height.value;
    }
    if (ramp_index >= 0) {
      const RampDef& ramp = events_.map().ramps[static_cast<std::size_t>(ramp_index)];
      ramp_direction_index_ = static_cast<int>(ramp.direction);
      ramp_low_y_ = ramp.low_y;
      ramp_high_y_ = ramp.high_y;
    } else if (tile_height.ok) {
      ramp_low_y_ = tile_height.value;
      ramp_high_y_ = tile_height.value;
    }
  }

  if (tile_height.ok) {
    ImGui::Text("Ground Y: %.3f", tile_height.value);
    ImGui::TextUnformatted("Tile in range: yes");
  } else {
    ImGui::TextUnformatted("Ground Y: (out of range)");
    ImGui::TextUnformatted("Tile in range: no");
  }

  const bool tile_has_ramp = ramp_index >= 0;
  if (tile_has_ramp) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("- Step")) {
    if (apply_result(events_.adjust_tile_elevation(height_tile_x_, height_tile_z_, -height_step_))) {
      const HeightGetResult updated =
          get_tile_ground_y(events_.map().height_grid, height_tile_x_, height_tile_z_);
      if (updated.ok) {
        height_set_y_ = updated.value;
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("+ Step")) {
    if (apply_result(events_.adjust_tile_elevation(height_tile_x_, height_tile_z_, height_step_))) {
      const HeightGetResult updated =
          get_tile_ground_y(events_.map().height_grid, height_tile_x_, height_tile_z_);
      if (updated.ok) {
        height_set_y_ = updated.value;
      }
    }
  }
  if (tile_has_ramp) {
    ImGui::EndDisabled();
  }
  ImGui::InputFloat("Set Y", &height_set_y_, 0.05f, 0.25f, "%.3f");
  if (tile_has_ramp) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("Set tile height")) {
    (void)apply_result(events_.set_tile_elevation(height_tile_x_, height_tile_z_, height_set_y_));
  }
  if (tile_has_ramp) {
    ImGui::EndDisabled();
  }

  ImGui::Separator();
  if (ramp_index >= 0) {
    ImGui::TextUnformatted("Ramp on tile: yes");
    ImGui::TextUnformatted("Flat ground editing disabled; use ramp controls.");
  } else {
    ImGui::TextUnformatted("Ramp on tile: no");
  }

  ImGui::Combo("Ramp direction", &ramp_direction_index_, "North\0East\0South\0West\0");
  ImGui::InputFloat("Ramp low Y", &ramp_low_y_, 0.05f, 0.25f, "%.3f");
  ImGui::InputFloat("Ramp high Y", &ramp_high_y_, 0.05f, 0.25f, "%.3f");

  if (ImGui::Button(ramp_index >= 0 ? "Update ramp" : "Add ramp")) {
    RampDef ramp;
    ramp.tile = ramp_tile;
    ramp.direction = static_cast<RampDirection>(ramp_direction_index_);
    ramp.low_y = ramp_low_y_;
    ramp.high_y = ramp_high_y_;
    (void)apply_result(events_.upsert_ramp_elevation(ramp));
  }
  ImGui::SameLine();
  if (ImGui::Button("Remove ramp")) {
    (void)apply_result(events_.remove_ramp_elevation(ramp_tile));
  }
}

void EditorApp::draw_event_edit_ui() {
  ImGui::Separator();
  ImGui::TextUnformatted("Events (Edit)");
  const float tile = events_.map().tile_size > 0.0f ? events_.map().tile_size : 1.0f;
  auto event_list = events_.map().events;
  auto run_history = [&](std::unique_ptr<EditCommand> command) {
    MapData map = events_.map();
    const EditApplyResult result = edit_history_.execute(map, std::move(command));
    apply_edited_map(std::move(map), result);
    event_list = events_.map().events;
  };

  if (ImGui::Button("Add stub event")) {
    const std::string id = "stub_" + std::to_string(next_stub_event_++);
    run_history(make_place_event_command(make_stub_event(id, 0, 0)));
    selected_event_ = static_cast<int>(event_list.size()) - 1;
    selected_page_ = 0;
    sync_events_to_runtime();
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete event") && selected_event_ >= 0 &&
      selected_event_ < static_cast<int>(event_list.size())) {
    run_history(make_delete_event_command(static_cast<std::size_t>(selected_event_)));
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
    ImGui::Text("Why not: %s",
                event_why_not_name(events_.why_not_fired(event.id, game_state_, player_, false)));

    if (!event.tile.has_value() && !event.volume.has_value()) {
      if (ImGui::Button("Place on tile (0,0)")) {
        event.tile = TileCoord{0, 0};
        apply_event(std::move(event));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
    } else {
      if (ImGui::Button("Ev -X")) {
        run_history(make_move_event_command(static_cast<std::size_t>(selected_event_), -1, 0, tile));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::SameLine();
      if (ImGui::Button("Ev +X")) {
        run_history(make_move_event_command(static_cast<std::size_t>(selected_event_), 1, 0, tile));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::SameLine();
      if (ImGui::Button("Ev -Z")) {
        run_history(make_move_event_command(static_cast<std::size_t>(selected_event_), 0, -1, tile));
        event = event_list[static_cast<std::size_t>(selected_event_)];
      }
      ImGui::SameLine();
      if (ImGui::Button("Ev +Z")) {
        run_history(make_move_event_command(static_cast<std::size_t>(selected_event_), 0, 1, tile));
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
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("hot-apply failed (") + path + "): " + result.error);
    }
    return false;
  }

  last_apply_error_.clear();
  map_path_ = path;
  selected_blocker_ = blockers.empty() ? -1 : 0;
  selected_event_ = events_.map().events.empty() ? -1 : 0;
  selected_page_ = 0;
  height_tile_x_ = events_.map().height_grid.origin_x;
  height_tile_z_ = events_.map().height_grid.origin_z;
  height_step_ = 0.25f;
  height_set_y_ = 0.0f;
  ramp_direction_index_ = 0;
  ramp_low_y_ = 0.0f;
  ramp_high_y_ = 0.0f;
  height_tile_sync_ready_ = false;
  last_height_tile_x_ = 0;
  last_height_tile_z_ = 0;
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
  previous_buttons_ = window_ != nullptr ? sample_editor_buttons(window_) : InputButtons{};
  clear_buffered_press(interact_press_buffer_);
  jump_press_pending_ = false;
  fixed_accumulator_ = 0.0f;
  snap_player_to_ground_clear_jump();
  edit_history_.clear();
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
    if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "editor",
          std::string("map save failed (") + path + "): " + result.error);
    }
    return false;
  }
  last_apply_error_.clear();
  last_serialize_status_ = "Saved: " + path;
  return true;
}

bool EditorApp::init() {
  file_log_ = std::make_unique<FileLogSink>(default_log_path());
  stderr_log_ = std::make_unique<StreamLogSink>(std::cerr);
  tee_log_ = std::make_unique<TeeLogSink>(*file_log_, *stderr_log_);
  logger_ = std::make_unique<Logger>(*tee_log_);
  audio_sink_ = std::make_unique<LogAudioSink>(*logger_);
  audio_ = std::make_unique<QueuedAudio>(*audio_sink_);
  events_.set_audio(audio_.get());
  events_.set_notify(&notify_bus_);
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

  if (!glfwInit()) {
    log(*logger_, LogLevel::Error, "editor", "glfwInit failed");
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(width_, height_, "rat-editor", nullptr, nullptr);
  if (window_ == nullptr) {
    log(*logger_, LogLevel::Error, "editor", "glfwCreateWindow failed");
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
    log(*logger_, LogLevel::Error, "editor", "ImGui_ImplGlfw_InitForOther failed");
    shutdown();
    return false;
  }

  if (!imgui_bgfx::init(255)) {
    log(*logger_, LogLevel::Error, "editor", "imgui_bgfx::init failed");
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
    if (audio_ != nullptr) {
      audio_->drain();
    }

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

  const InputButtons buttons = sample_editor_buttons(window_);
  InputGating gating;
  gating.player_control = player_control_enabled(app_mode_);
  gating.keyboard_captured = io.WantCaptureKeyboard;
  gating.dialog_open = events_.active_message().has_value();
  gating.player_input_blocked = events_.player_input_blocked();
  const InputFrame input = map_input_frame(buttons, previous_buttons_, gating);
  previous_buttons_ = buttons;

  if (input.toggle_mode_pressed) {
    set_app_mode(toggle_app_mode(app_mode_));
  }

  if (input.hot_apply_pressed && !map_path_.empty()) {
    clear_buffered_press(interact_press_buffer_);
    hot_apply_map_path(map_path_, true);
  }

  if (app_mode_ == AppMode::Edit) {
    if (input.undo_pressed) {
      MapData map = events_.map();
      const EditApplyResult result = edit_history_.undo(map);
      if (result.applied) {
        apply_edited_map(std::move(map), result);
      }
    }
    if (input.redo_pressed) {
      MapData map = events_.map();
      const EditApplyResult result = edit_history_.redo(map);
      if (result.applied) {
        apply_edited_map(std::move(map), result);
      }
    }
  }

  push_buffered_press_if_allowed(interact_press_buffer_, input.interact_pressed,
                                 event_runtime_enabled(app_mode_), io.WantCaptureKeyboard, 0.1f);
  clear_buffered_press_if_captured(interact_press_buffer_, io.WantCaptureKeyboard);

  if (input.jump_pressed) {
    jump_press_pending_ = true;
  }

  bool consumed_for_dialog = false;
  if (event_runtime_enabled(app_mode_) && events_.active_message().has_value() && !io.WantCaptureKeyboard &&
      consume_buffered_press(interact_press_buffer_)) {
    events_.acknowledge_message();
    consumed_for_dialog = true;
  }

  if (input.cycle_camera_pressed) {
    engine_->greybox().set_camera_mode(next_camera_mode(engine_->greybox().camera_mode()));
  }

  if (input.debug_snapshot_pressed) {
    const DebugSnapshot snapshot =
        make_debug_snapshot(sim_frame_, app_mode_, player_, jump_state_, events_, game_state_,
                            input.interact_pressed);
    const std::string path = default_debug_snapshot_path();
    if (write_debug_snapshot(path, snapshot)) {
      if (logger_ != nullptr) {
        log(*logger_, LogLevel::Info, "debug", std::string("wrote snapshot ") + path);
      }
    } else if (logger_ != nullptr) {
      log(*logger_, LogLevel::Error, "debug", std::string("failed to write snapshot ") + path);
    }
  }

  fixed_accumulator_ += std::max(0.0f, dt);
  fixed_accumulator_ = std::min(fixed_accumulator_, 0.2f);
  while (fixed_accumulator_ >= kFixedStep) {
    fixed_accumulator_ -= kFixedStep;
    ++sim_frame_;

    const bool allow_player_input =
        player_control_enabled(app_mode_) && !io.WantCaptureKeyboard && !events_.player_input_blocked();
    if (!allow_player_input) {
      jump_state_.jump_buffer_left = 0.0f;
      jump_press_pending_ = false;
    }

    if (player_control_enabled(app_mode_)) {
      PlayerFrameInput frame_input = player_input_from_frame(input);
      frame_input.jump_pressed = allow_player_input && jump_press_pending_;
      frame_input.jump_held = allow_player_input && input.jump_held;

      if (surface_query_cache_ == nullptr) {
        rebuild_surface_query_cache();
      }
      const PlayerFrameResult frame = integrate_player_frame_surface(
          player_, jump_state_, frame_input, kFixedStep, engine_->blockers(), *surface_query_cache_,
          jump_tuning_);
      player_ = frame.body;
      jump_state_ = frame.jump;
      if (frame.landed) {
        notify_bus_.post({GameplayNotifyKind::Landed, {}});
      }
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
        previous_buttons_.jump = buttons.jump;
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
    ImGui::TextUnformatted("EDIT: player + events paused. Edit elevation/blockers/events below.");
  } else {
    ImGui::TextUnformatted("Quest: ask foreman (cyan) for a rusty cog,");
    ImGui::TextUnformatted("loot scrap east of crates, return.");
  }
  ImGui::TextUnformatted("WASD move | Space jump | E interact | C camera | F2 Play/Edit | F3 snapshot | F5 hot-apply");
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
    draw_height_edit_ui();
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
    const float tile_size = events_.map().tile_size > 0.0f ? events_.map().tile_size : 1.0f;
    const auto snapped = snap_to_grid(player_.x, player_.y, player_.z, tile_size);
    player_.x = snapped.x;
    player_.z = snapped.z;
    snap_player_to_ground_clear_jump();
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
