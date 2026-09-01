#include "blocker_panel.hpp"

#include <rat/blocker_edit.hpp>
#include <rat/edit_history.hpp>

#include <imgui.h>

#include <cstdio>
#include <utility>

namespace rat {

void draw_blocker_panel(EditorDocument& document, BlockerPanelState& state, float sampled_ground_y,
                        std::string& last_error) {
  ImGui::Separator();
  ImGui::TextUnformatted("Blockers (Edit)");
  const float tile = document.visible_data().tile_size > 0.0f ? document.visible_data().tile_size
                                                              : 1.0f;

  if (ImGui::Button("Add blocker")) {
    BlockerDef blocker;
    blocker.bounds = Aabb2{0.0f, 0.0f, tile, tile};
    state.field_origin.reset();
    state.field_origin_index = -1;
    (void)document.execute(make_place_blocker_command(std::move(blocker)));
    document.select_blocker(static_cast<int>(document.data().blockers.size()) - 1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete selected") && document.selected_blocker() >= 0 &&
      document.selected_blocker() < static_cast<int>(document.visible_data().blockers.size())) {
    state.field_origin.reset();
    state.field_origin_index = -1;
    (void)document.execute(
        make_delete_blocker_command(static_cast<std::size_t>(document.selected_blocker())));
  }

  const auto& blockers = document.visible_data().blockers;
  if (ImGui::BeginListBox("##blockers", ImVec2(-1.0f, 100.0f))) {
    for (int i = 0; i < static_cast<int>(blockers.size()); ++i) {
      const auto& b = blockers[static_cast<std::size_t>(i)].bounds;
      char label[128];
      std::snprintf(label, sizeof(label), "%d: (%.0f,%.0f)-(%.0f,%.0f)", i, b.min_x, b.min_z,
                    b.max_x, b.max_z);
      if (ImGui::Selectable(label, document.selected_blocker() == i)) {
        document.select_blocker(i);
      }
    }
    ImGui::EndListBox();
  }

  if (document.selected_blocker() < 0 ||
      document.selected_blocker() >= static_cast<int>(blockers.size())) {
    return;
  }

  auto selected_valid = [&]() {
    return document.selected_blocker() >= 0 &&
           document.selected_blocker() < static_cast<int>(document.visible_data().blockers.size());
  };
  auto replace_selected_blocker = [&](BlockerDef next) {
    if (!selected_valid()) {
      return;
    }
    state.field_origin.reset();
    state.field_origin_index = -1;
    (void)document.execute(make_replace_blocker_command(
        static_cast<std::size_t>(document.selected_blocker()), std::move(next)));
  };
  auto preview_selected_blocker = [&](BlockerDef next) {
    if (!selected_valid()) {
      return;
    }
    (void)document.preview_blocker(document.selected_blocker(), std::move(next));
  };
  auto commit_blocker_field_edit = [&]() {
    if (!ImGui::IsItemDeactivatedAfterEdit() || !state.field_origin || !selected_valid()) {
      return;
    }
    BlockerDef next =
        document.visible_data().blockers[static_cast<std::size_t>(document.selected_blocker())];
    state.field_origin.reset();
    replace_selected_blocker(std::move(next));
  };

  if (state.field_origin_index != document.selected_blocker()) {
    state.field_origin.reset();
    state.field_origin_index = document.selected_blocker();
  }

  ImGui::Text("Selected %d", document.selected_blocker());
  if (!selected_valid()) {
    return;
  }
  BlockerDef current =
      document.visible_data().blockers[static_cast<std::size_t>(document.selected_blocker())];
  const Aabb2 box = current.bounds;

  if (ImGui::Button("-X")) {
    state.field_origin.reset();
    (void)document.execute(make_move_blocker_command(
        static_cast<std::size_t>(document.selected_blocker()), -1, 0, tile));
  }
  ImGui::SameLine();
  if (ImGui::Button("+X")) {
    state.field_origin.reset();
    (void)document.execute(make_move_blocker_command(
        static_cast<std::size_t>(document.selected_blocker()), 1, 0, tile));
  }
  ImGui::SameLine();
  if (ImGui::Button("-Z")) {
    state.field_origin.reset();
    (void)document.execute(make_move_blocker_command(
        static_cast<std::size_t>(document.selected_blocker()), 0, -1, tile));
  }
  ImGui::SameLine();
  if (ImGui::Button("+Z")) {
    state.field_origin.reset();
    (void)document.execute(make_move_blocker_command(
        static_cast<std::size_t>(document.selected_blocker()), 0, 1, tile));
  }

  if (ImGui::Button("Grow +X")) {
    BlockerDef next = current;
    next.bounds = resize_aabb_on_grid(box, AabbEdge::MaxX, 1, tile);
    replace_selected_blocker(std::move(next));
  }
  ImGui::SameLine();
  if (ImGui::Button("Shrink +X")) {
    BlockerDef next = current;
    next.bounds = resize_aabb_on_grid(box, AabbEdge::MaxX, -1, tile);
    replace_selected_blocker(std::move(next));
  }
  if (ImGui::Button("Grow +Z")) {
    BlockerDef next = current;
    next.bounds = resize_aabb_on_grid(box, AabbEdge::MaxZ, 1, tile);
    replace_selected_blocker(std::move(next));
  }
  ImGui::SameLine();
  if (ImGui::Button("Shrink +Z")) {
    BlockerDef next = current;
    next.bounds = resize_aabb_on_grid(box, AabbEdge::MaxZ, -1, tile);
    replace_selected_blocker(std::move(next));
  }
  if (ImGui::Button("Snap to grid")) {
    BlockerDef next = current;
    next.bounds = snap_aabb_to_grid(box, tile);
    replace_selected_blocker(std::move(next));
  }

  if (!selected_valid()) {
    return;
  }
  current = document.visible_data().blockers[static_cast<std::size_t>(document.selected_blocker())];

  ImGui::Separator();
  bool jumpable = current.jumpable;
  if (ImGui::Checkbox("Jumpable vertical blocker", &jumpable)) {
    BlockerDef updated = current;
    if (jumpable) {
      const std::optional<float> base =
          updated.base_y.has_value() ? updated.base_y : sampled_ground_y;
      const std::optional<float> top =
          updated.top_y.has_value() ? updated.top_y : (sampled_ground_y + 0.9f);
      if (!set_blocker_vertical_range(updated, true, base, top)) {
        last_error = "Invalid blocker vertical pair";
      } else {
        last_error.clear();
        replace_selected_blocker(std::move(updated));
      }
    } else {
      (void)set_blocker_vertical_range(updated, false, std::nullopt, std::nullopt);
      last_error.clear();
      replace_selected_blocker(std::move(updated));
    }
  }

  if (!selected_valid()) {
    return;
  }
  current = document.visible_data().blockers[static_cast<std::size_t>(document.selected_blocker())];

  if (current.jumpable) {
    float base_y = current.base_y.value_or(0.0f);
    float top_y = current.top_y.value_or(base_y + 0.9f);
    auto preview_vertical = [&]() {
      if (top_y < base_y) {
        top_y = base_y;
      }
      BlockerDef updated = current;
      if (!set_blocker_vertical_range(updated, true, base_y, top_y)) {
        last_error = "Invalid blocker vertical pair";
        return;
      }
      last_error.clear();
      if (!state.field_origin) {
        state.field_origin = current;
        state.field_origin_index = document.selected_blocker();
      }
      preview_selected_blocker(std::move(updated));
      current =
          document.visible_data().blockers[static_cast<std::size_t>(document.selected_blocker())];
    };
    if (ImGui::InputFloat("Base Y", &base_y, 0.05f, 0.25f, "%.3f")) {
      preview_vertical();
    }
    commit_blocker_field_edit();
    if (ImGui::InputFloat("Top Y", &top_y, 0.05f, 0.25f, "%.3f")) {
      preview_vertical();
    }
    commit_blocker_field_edit();
  } else {
    ImGui::TextUnformatted("Legacy full wall (no vertical pair).");
  }
}

}  // namespace rat
