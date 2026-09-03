#include "event_graph_window.hpp"

#include "event_graph_canvas.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_inspect.hpp>
#include <rat/map_data.hpp>

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

namespace rat {

void draw_event_graph_window(EditorDocument& document, EventPanelState& state, bool* open) {
  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.visible_data().events.size())) {
    return;
  }

  if (!ImGui::Begin("Event Graph", open)) {
    ImGui::End();
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
  auto reload_event = [&]() -> EventDef {
    return document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
  };

  if (state.field_origin_index != document.selected_event()) {
    state.field_origin.reset();
    state.field_origin_index = document.selected_event();
  }

  EventDef event = reload_event();
  ImGui::Text("id: %s", event.id.c_str());

  if (event.pages.empty()) {
    if (ImGui::Button("Add page")) {
      const int idx = add_event_page(event);
      replace_selected_event(event);
      document.set_selected_page(idx);
      state.graph_tab_select = idx;
    }
    ImGui::End();
    return;
  }

  if (document.selected_page() < 0 ||
      document.selected_page() >= static_cast<int>(event.pages.size())) {
    document.set_selected_page(0);
  }

  const int force_tab = state.graph_tab_select;
  if (state.graph_tab_select >= 0) {
    state.graph_tab_select = -1;
  }

  if (ImGui::BeginTabBar("##event_pages")) {
    bool mutated = false;
    for (int i = 0; i < static_cast<int>(event.pages.size()); ++i) {
      char label[32];
      std::snprintf(label, sizeof(label), "Page %d", i + 1);
      ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
      if (force_tab == i) {
        flags |= ImGuiTabItemFlags_SetSelected;
      }
      ImGui::PushID(i);
      if (ImGui::BeginTabItem(label, nullptr, flags)) {
        if (document.selected_page() != i) {
          document.set_selected_page(i);
        }
        if (ImGui::BeginPopupContextItem("##page_tab_menu")) {
          if (ImGui::MenuItem("Add page")) {
            const int idx = add_event_page(event);
            replace_selected_event(event);
            document.set_selected_page(idx);
            state.graph_tab_select = idx;
            mutated = true;
          }
          if (ImGui::MenuItem("Duplicate page")) {
            const int idx = duplicate_event_page(event, static_cast<std::size_t>(i));
            if (idx >= 0) {
              replace_selected_event(event);
              document.set_selected_page(idx);
              state.graph_tab_select = idx;
              mutated = true;
            }
          }
          const bool can_delete = event.pages.size() > 1;
          if (ImGui::MenuItem("Delete page", nullptr, false, can_delete) && can_delete) {
            const int sel = document.selected_page();
            if (remove_event_page(event, static_cast<std::size_t>(i))) {
              replace_selected_event(event);
              event = reload_event();
              int next = sel;
              if (sel > i) {
                next = sel - 1;
              } else if (sel == i) {
                next = (std::min)(sel, static_cast<int>(event.pages.size()) - 1);
              }
              if (next < 0) {
                next = 0;
              }
              document.set_selected_page(next);
              state.graph_tab_select = next;
              mutated = true;
            }
          }
          ImGui::EndPopup();
        }
        ImGui::EndTabItem();
      }
      ImGui::PopID();
      if (mutated) {
        break;
      }
    }
    ImGui::EndTabBar();
  }

  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.visible_data().events.size())) {
    ImGui::End();
    return;
  }
  event = reload_event();
  if (event.pages.empty()) {
    ImGui::End();
    return;
  }
  if (document.selected_page() < 0 ||
      document.selected_page() >= static_cast<int>(event.pages.size())) {
    document.set_selected_page(0);
  }

  EventPage& page = event.pages[static_cast<std::size_t>(document.selected_page())];
  int trigger = static_cast<int>(page.trigger);
  if (ImGui::Combo("Trigger", &trigger, "action\0player_touch\0event_touch\0autorun\0parallel\0")) {
    page.trigger = static_cast<TriggerKind>(trigger);
    replace_selected_event(event);
    event = reload_event();
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Conditions (AND)");
  int remove_cond = -1;
  const int n_conds = static_cast<int>(
      event.pages[static_cast<std::size_t>(document.selected_page())].conditions.size());
  for (int i = 0; i < n_conds; ++i) {
    ImGui::PushID(i);
    Condition& cond = event.pages[static_cast<std::size_t>(document.selected_page())]
                          .conditions[static_cast<std::size_t>(i)];
    int type = static_cast<int>(cond.type);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo("##type", &type, "Switch\0Variable\0Item\0Self Switch\0")) {
      cond.type = static_cast<ConditionType>(type);
      replace_selected_event(event);
      event = reload_event();
    }
    Condition& live = event.pages[static_cast<std::size_t>(document.selected_page())]
                          .conditions[static_cast<std::size_t>(i)];
    ImGui::SameLine();
    switch (live.type) {
      case ConditionType::Switch: {
        int switch_id = static_cast<int>(live.id);
        const EventDef snapshot = event;
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputInt("##sw_id", &switch_id)) {
          if (switch_id < 0) {
            switch_id = 0;
          }
          live.id = static_cast<std::uint32_t>(switch_id);
          if (!state.field_origin) {
            state.field_origin = snapshot;
            state.field_origin_index = document.selected_event();
          }
          preview_selected_event(event);
          event = reload_event();
        }
        commit_event_field_edit();
        event = reload_event();
        Condition& sw = event.pages[static_cast<std::size_t>(document.selected_page())]
                            .conditions[static_cast<std::size_t>(i)];
        ImGui::SameLine();
        bool on = sw.bool_value;
        if (ImGui::Checkbox("ON", &on)) {
          sw.bool_value = on;
          replace_selected_event(event);
          event = reload_event();
        }
        break;
      }
      case ConditionType::Variable: {
        int var_id = static_cast<int>(live.id);
        const EventDef snapshot = event;
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputInt("##var_id", &var_id)) {
          if (var_id < 0) {
            var_id = 0;
          }
          live.id = static_cast<std::uint32_t>(var_id);
          if (!state.field_origin) {
            state.field_origin = snapshot;
            state.field_origin_index = document.selected_event();
          }
          preview_selected_event(event);
          event = reload_event();
        }
        commit_event_field_edit();
        event = reload_event();
        Condition& var = event.pages[static_cast<std::size_t>(document.selected_page())]
                             .conditions[static_cast<std::size_t>(i)];
        int op = static_cast<int>(var.op);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60.0f);
        if (ImGui::Combo("##op", &op, "Eq\0Ne\0Lt\0Le\0Gt\0Ge\0")) {
          var.op = static_cast<CompareOp>(op);
          replace_selected_event(event);
          event = reload_event();
        }
        Condition& var_val = event.pages[static_cast<std::size_t>(document.selected_page())]
                                 .conditions[static_cast<std::size_t>(i)];
        int value = var_val.int_value;
        const EventDef value_snapshot = event;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputInt("##var_val", &value)) {
          var_val.int_value = value;
          if (!state.field_origin) {
            state.field_origin = value_snapshot;
            state.field_origin_index = document.selected_event();
          }
          preview_selected_event(event);
          event = reload_event();
        }
        commit_event_field_edit();
        event = reload_event();
        break;
      }
      case ConditionType::Item: {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s", live.string_id.c_str());
        const EventDef snapshot = event;
        ImGui::SetNextItemWidth(140.0f);
        if (ImGui::InputText("##item_id", buf, sizeof(buf))) {
          live.string_id = buf;
          if (!state.field_origin) {
            state.field_origin = snapshot;
            state.field_origin_index = document.selected_event();
          }
          preview_selected_event(event);
          event = reload_event();
        }
        commit_event_field_edit();
        event = reload_event();
        Condition& item = event.pages[static_cast<std::size_t>(document.selected_page())]
                              .conditions[static_cast<std::size_t>(i)];
        int qty = item.int_value;
        const EventDef qty_snapshot = event;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::InputInt("##item_qty", &qty)) {
          if (qty < 0) {
            qty = 0;
          }
          item.int_value = qty;
          if (!state.field_origin) {
            state.field_origin = qty_snapshot;
            state.field_origin_index = document.selected_event();
          }
          preview_selected_event(event);
          event = reload_event();
        }
        commit_event_field_edit();
        event = reload_event();
        break;
      }
      case ConditionType::SelfSwitch: {
        int ss = live.self_switch - 'A';
        if (ss < 0 || ss > 3) {
          ss = 0;
        }
        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::Combo("##self", &ss, "A\0B\0C\0D\0")) {
          live.self_switch = static_cast<char>('A' + ss);
          replace_selected_event(event);
          event = reload_event();
        }
        Condition& self = event.pages[static_cast<std::size_t>(document.selected_page())]
                              .conditions[static_cast<std::size_t>(i)];
        ImGui::SameLine();
        bool on = self.bool_value;
        if (ImGui::Checkbox("ON", &on)) {
          self.bool_value = on;
          replace_selected_event(event);
          event = reload_event();
        }
        break;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove")) {
      remove_cond = i;
    }
    ImGui::PopID();
  }

  if (remove_cond >= 0) {
    EventPage& page_rm = event.pages[static_cast<std::size_t>(document.selected_page())];
    if (remove_page_condition(page_rm, static_cast<std::size_t>(remove_cond))) {
      replace_selected_event(event);
      event = reload_event();
    }
  }

  auto selected_page = [&]() -> EventPage& {
    return event.pages[static_cast<std::size_t>(document.selected_page())];
  };
  if (ImGui::Button("Add Switch")) {
    (void)add_page_condition(selected_page(), ConditionType::Switch);
    replace_selected_event(event);
    event = reload_event();
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Variable")) {
    (void)add_page_condition(selected_page(), ConditionType::Variable);
    replace_selected_event(event);
    event = reload_event();
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Item")) {
    (void)add_page_condition(selected_page(), ConditionType::Item);
    replace_selected_event(event);
    event = reload_event();
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Self Switch")) {
    (void)add_page_condition(selected_page(), ConditionType::SelfSwitch);
    replace_selected_event(event);
    event = reload_event();
  }

  draw_event_graph_canvas(document, state.canvas, event, state.last_compile_error);
  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.visible_data().events.size())) {
    ImGui::End();
    return;
  }
  event = reload_event();
  if (document.selected_page() < 0 ||
      document.selected_page() >= static_cast<int>(event.pages.size())) {
    ImGui::End();
    return;
  }
  EventPage& page_list = event.pages[static_cast<std::size_t>(document.selected_page())];
  if (!page_list.graph.has_value()) {
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

  ImGui::End();
}

}  // namespace rat
