#include "event_panel.hpp"

#include "event_graph_canvas.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_inspect.hpp>
#include <rat/map_data.hpp>

#include <imgui.h>

#include <cstdio>
#include <string>
#include <utility>

namespace rat {

void draw_event_panel(EditorDocument& document, EventPanelState& state, const char* why_not) {
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

  ImGui::Separator();
  ImGui::TextUnformatted("Page inspector");
  if (event.pages.empty()) {
    ImGui::TextUnformatted("(no pages)");
    return;
  }
  if (document.selected_page() < 0 ||
      document.selected_page() >= static_cast<int>(event.pages.size())) {
    document.set_selected_page(0);
  }
  if (ImGui::BeginListBox("##pages", ImVec2(-1.0f, 80.0f))) {
    for (int p = 0; p < static_cast<int>(event.pages.size()); ++p) {
      const EventPage& page = event.pages[static_cast<std::size_t>(p)];
      char label[192];
      std::snprintf(label, sizeof(label), "%d: %s | %s", p, trigger_kind_name(page.trigger),
                    summarize_page_conditions(page).c_str());
      if (ImGui::Selectable(label, document.selected_page() == p)) {
        document.set_selected_page(p);
      }
    }
    ImGui::EndListBox();
  }

  EventPage& page = event.pages[static_cast<std::size_t>(document.selected_page())];
  int trigger = static_cast<int>(page.trigger);
  if (ImGui::Combo("Trigger", &trigger, "action\0player_touch\0event_touch\0autorun\0parallel\0")) {
    page.trigger = static_cast<TriggerKind>(trigger);
    replace_selected_event(event);
    event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
  }
  EventPage& page_inspect = event.pages[static_cast<std::size_t>(document.selected_page())];
  ImGui::TextWrapped("Conditions: %s", summarize_page_conditions(page_inspect).c_str());

  int sw_i = -1;
  for (int i = 0; i < static_cast<int>(page_inspect.conditions.size()); ++i) {
    if (page_inspect.conditions[static_cast<std::size_t>(i)].type == ConditionType::Switch) {
      sw_i = i;
      break;
    }
  }
  if (sw_i < 0) {
    if (ImGui::Button("Add enable Switch")) {
      (void)ensure_page_enable_switch(page_inspect, 1, true);
      replace_selected_event(event);
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
  } else {
    Condition& sw = event.pages[static_cast<std::size_t>(document.selected_page())]
                        .conditions[static_cast<std::size_t>(sw_i)];
    int switch_id = static_cast<int>(sw.id);
    const EventDef switch_snapshot = event;
    if (ImGui::InputInt("Enable SW id", &switch_id)) {
      if (switch_id < 0) {
        switch_id = 0;
      }
      sw.id = static_cast<std::uint32_t>(switch_id);
      if (!state.field_origin) {
        state.field_origin = switch_snapshot;
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
    Condition& sw_now = event.pages[static_cast<std::size_t>(document.selected_page())]
                            .conditions[static_cast<std::size_t>(sw_i)];
    bool on = sw_now.bool_value;
    if (ImGui::Checkbox("Enable SW ON", &on)) {
      sw_now.bool_value = on;
      replace_selected_event(event);
      event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
    }
  }

  draw_event_graph_canvas(document, state.canvas, event, state.last_compile_error);
  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.visible_data().events.size())) {
    return;
  }
  event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
  EventPage& page_list = event.pages[static_cast<std::size_t>(document.selected_page())];
  if (page_list.graph.has_value()) {
    return;
  }
  int text_i = find_first_show_text(page_list);
  if (text_i < 0) {
    if (ImGui::Button("Add Show Text")) {
      Command cmd;
      cmd.op = CommandOp::ShowText;
      cmd.text = "New text";
      page_list.commands.insert(page_list.commands.begin(), std::move(cmd));
      replace_selected_event(event);
    }
  } else {
    Command& cmd = event.pages[static_cast<std::size_t>(document.selected_page())]
                       .commands[static_cast<std::size_t>(text_i)];
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s", cmd.text.c_str());
    const EventDef text_snapshot = event;
    if (ImGui::InputTextMultiline("Show Text", buf, sizeof(buf), ImVec2(-1.0f, 60.0f))) {
      cmd.text = buf;
      if (!state.field_origin) {
        state.field_origin = text_snapshot;
        state.field_origin_index = document.selected_event();
      }
      preview_selected_event(event);
    }
    commit_event_field_edit();
  }
}

}  // namespace rat
