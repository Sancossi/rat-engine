#include "event_panel.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/map_data.hpp>

#include <imgui.h>

#include <cstdio>
#include <optional>
#include <string>
#include <utility>

namespace rat {

void draw_event_panel(EditorDocument& document, EventPanelState& state, const char* why_not,
                      bool* event_graph_open) {
  ImGui::Separator();
  ImGui::TextUnformatted("Events (Edit)");
  const float tile = document.visible_data().tile_size > 0.0f ? document.visible_data().tile_size
                                                              : 1.0f;

  if (ImGui::Button("Add stub event")) {
    const std::string id = "stub_" + std::to_string(state.next_stub_event++);
    state.field_origin.reset();
    state.field_origin_index = -1;
    (void)document.execute(make_place_event_command(make_stub_event(id, 0, 0)));
    document.select_event(static_cast<int>(document.data().events.size()) - 1);
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete event") && document.selected_event() >= 0 &&
      document.selected_event() < static_cast<int>(document.visible_data().events.size())) {
    state.field_origin.reset();
    state.field_origin_index = -1;
    (void)document.execute(
        make_delete_event_command(static_cast<std::size_t>(document.selected_event())));
  }

  const auto& event_list = document.visible_data().events;
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
      if (ImGui::Selectable(label, document.selected_event() == i)) {
        document.select_event(i);
      }
    }
    ImGui::EndListBox();
  }

  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.visible_data().events.size())) {
    return;
  }

  auto replace_selected_event = [&](EventDef next) {
    state.field_origin.reset();
    state.field_origin_index = -1;
    (void)document.execute(make_replace_event_command(
        static_cast<std::size_t>(document.selected_event()), std::move(next)));
  };
  auto preview_selected_event = [&](EventDef next) {
    (void)document.preview_event(document.selected_event(), std::move(next));
  };
  auto commit_event_field_edit = [&]() {
    if (!ImGui::IsItemDeactivatedAfterEdit() || !state.field_origin) {
      return;
    }
    EventDef next =
        document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    state.field_origin.reset();
    replace_selected_event(std::move(next));
  };

  if (state.field_origin_index != document.selected_event()) {
    state.field_origin.reset();
    state.field_origin_index = document.selected_event();
  }

  EventDef event =
      document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
  ImGui::Text("id: %s", event.id.c_str());
  ImGui::Text("pages: %zu", event.pages.size());
  ImGui::Text("Why not: %s", why_not != nullptr ? why_not : "");

  if (!event.tile.has_value() && !event.volume.has_value()) {
    if (ImGui::Button("Place on tile (0,0)")) {
      event.tile = TileCoord{0, 0};
      replace_selected_event(std::move(event));
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
  } else {
    if (ImGui::Button("Ev -X")) {
      state.field_origin.reset();
      (void)document.execute(make_move_event_command(
          static_cast<std::size_t>(document.selected_event()), -1, 0, tile));
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
    ImGui::SameLine();
    if (ImGui::Button("Ev +X")) {
      state.field_origin.reset();
      (void)document.execute(make_move_event_command(
          static_cast<std::size_t>(document.selected_event()), 1, 0, tile));
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
    ImGui::SameLine();
    if (ImGui::Button("Ev -Z")) {
      state.field_origin.reset();
      (void)document.execute(make_move_event_command(
          static_cast<std::size_t>(document.selected_event()), 0, -1, tile));
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
    ImGui::SameLine();
    if (ImGui::Button("Ev +Z")) {
      state.field_origin.reset();
      (void)document.execute(make_move_event_command(
          static_cast<std::size_t>(document.selected_event()), 0, 1, tile));
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
  }

  auto highest_slab_top = [&](TileCoord tile) -> std::optional<float> {
    std::optional<float> top;
    for (const FloorSlabDef& slab : document.visible_data().floor_slabs) {
      if (slab.tile.x != tile.x || slab.tile.z != tile.z) {
        continue;
      }
      if (!top.has_value() || slab.top_y > *top) {
        top = slab.top_y;
      }
    }
    return top;
  };

  bool bind_y = event.y.has_value();
  if (ImGui::Checkbox("Bind Y", &bind_y)) {
    if (bind_y) {
      event.y = 0.0f;
      if (event.tile.has_value()) {
        if (const std::optional<float> slab_top = highest_slab_top(*event.tile)) {
          event.y = *slab_top;
        }
      }
    } else {
      event.y.reset();
    }
    replace_selected_event(event);
    event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
  }
  if (event.y.has_value()) {
    float bind = *event.y;
    const EventDef y_snapshot = event;
    if (ImGui::InputFloat("Event Y", &bind, 0.05f, 0.25f, "%.3f")) {
      event.y = bind;
      if (!state.field_origin) {
        state.field_origin = y_snapshot;
        state.field_origin_index = document.selected_event();
      }
      preview_selected_event(event);
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
    commit_event_field_edit();
    if (document.selected_event() >= 0 &&
        document.selected_event() < static_cast<int>(document.visible_data().events.size())) {
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
    ImGui::SameLine();
    if (ImGui::Button("On slab") && event.tile.has_value()) {
      if (const std::optional<float> slab_top = highest_slab_top(*event.tile)) {
        event.y = *slab_top;
        replace_selected_event(event);
        event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
      }
    }
  }

  if (event_graph_open != nullptr && ImGui::Button("Open Event Graph")) {
    *event_graph_open = true;
  }
}

}  // namespace rat
