#include "event_graph_node_ui.hpp"

#include <imgui.h>

#include <climits>
#include <cstdint>
#include <cstdio>
#include <string>

namespace rat {
namespace {

bool labeled_input_int(const char* label, const char* id, int* value, int min_value) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(-1.0f);
  const bool changed = ImGui::InputInt(id, value, 1, 10);
  if (changed && *value < min_value) {
    *value = min_value;
  }
  return changed;
}

bool labeled_input_float(const char* label, const char* id, float* value) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(-1.0f);
  return ImGui::InputFloat(id, value, 0.0f, 0.0f, "%.2f");
}

bool labeled_input_text(const char* label, const char* id, char* buf, std::size_t buf_size) {
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(-1.0f);
  return ImGui::InputText(id, buf, buf_size);
}

}  // namespace

bool draw_event_graph_node_fields(EventGraphNode& node, float zoom) {
  bool commit = false;
  if (node.kind == "show_text" || node.kind == "comment") {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s", node.text.c_str());
    const ImVec2 size(-1.0f, 52.0f * zoom);
    if (ImGui::InputTextMultiline("##text", buf, sizeof(buf), size)) {
      node.text = buf;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      commit = true;
    }
  } else if (node.kind == "play_se") {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", node.text.c_str());
    if (labeled_input_text("Clip", "##clip", buf, sizeof(buf))) {
      node.text = buf;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      commit = true;
    }
  } else if (node.kind == "wait") {
    int frames = node.frames;
    if (labeled_input_int("Frames", "##frames", &frames, 0)) {
      node.frames = frames;
      commit = true;
    }
  } else if (node.kind == "control_switch") {
    int switch_id = static_cast<int>(node.switch_id);
    if (labeled_input_int("SW", "##sw", &switch_id, 0)) {
      node.switch_id = static_cast<std::uint32_t>(switch_id);
      commit = true;
    }
    bool on = node.bool_value;
    if (ImGui::Checkbox("ON", &on)) {
      node.bool_value = on;
      commit = true;
    }
  } else if (node.kind == "control_variable") {
    int var_id = static_cast<int>(node.switch_id);
    if (labeled_input_int("VAR", "##var", &var_id, 0)) {
      node.switch_id = static_cast<std::uint32_t>(var_id);
      commit = true;
    }
    int value = node.int_value;
    if (labeled_input_int("Value", "##val", &value, INT_MIN)) {
      node.int_value = value;
      commit = true;
    }
  } else if (node.kind == "control_self_switch") {
    int ss = node.self_switch - 'A';
    if (ss < 0 || ss > 3) {
      ss = 0;
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("SS");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##ss", &ss, "A\0B\0C\0D\0")) {
      node.self_switch = static_cast<char>('A' + ss);
      commit = true;
    }
    bool on = node.bool_value;
    if (ImGui::Checkbox("ON", &on)) {
      node.bool_value = on;
      commit = true;
    }
  } else if (node.kind == "conditional_branch") {
    int switch_id = static_cast<int>(node.branch_condition.id);
    if (labeled_input_int("If SW", "##ifsw", &switch_id, 0)) {
      node.branch_condition.type = ConditionType::Switch;
      node.branch_condition.id = static_cast<std::uint32_t>(switch_id);
      commit = true;
    }
    bool on = node.branch_condition.bool_value;
    if (ImGui::Checkbox("If SW ON", &on)) {
      node.branch_condition.bool_value = on;
      commit = true;
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Then");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Else");
  } else if (node.kind == "transfer_player") {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s", node.map_id.c_str());
    if (labeled_input_text("Map", "##map", buf, sizeof(buf))) {
      node.map_id = buf;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      commit = true;
    }
    float x = node.x;
    float y = node.y;
    float z = node.z;
    if (labeled_input_float("X", "##x", &x)) {
      node.x = x;
      commit = true;
    }
    if (labeled_input_float("Y", "##y", &y)) {
      node.y = y;
      commit = true;
    }
    if (labeled_input_float("Z", "##z", &z)) {
      node.z = z;
      commit = true;
    }
  } else if (node.kind == "change_items") {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s", node.item_id.c_str());
    if (labeled_input_text("Item", "##item", buf, sizeof(buf))) {
      node.item_id = buf;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      commit = true;
    }
    int delta = node.item_delta;
    if (labeled_input_int("Delta", "##delta", &delta, INT_MIN)) {
      node.item_delta = delta;
      commit = true;
    }
    bool key = node.key_item;
    if (ImGui::Checkbox("Key item", &key)) {
      node.key_item = key;
      commit = true;
    }
  } else if (node.kind == "set_move_route") {
    bool through = node.through;
    if (ImGui::Checkbox("Through", &through)) {
      node.through = through;
      commit = true;
    }
    const float combo_w = 90.0f * zoom;
    for (std::size_t i = 0; i < node.route.size(); ++i) {
      RouteStep& step = node.route[i];
      ImGui::PushID(static_cast<int>(i));
      int op = static_cast<int>(step.op);
      ImGui::SetNextItemWidth(combo_w);
      if (ImGui::Combo("##op", &op, "Move\0Wait\0Turn\0")) {
        step.op = static_cast<RouteStepOp>(op);
        commit = true;
      }
      if (step.op == RouteStepOp::Move || step.op == RouteStepOp::Turn) {
        int dir = static_cast<int>(step.dir);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##dir", &dir, "N\0E\0S\0W\0")) {
          step.dir = static_cast<RampDirection>(dir);
          commit = true;
        }
      } else if (step.op == RouteStepOp::Wait) {
        int frames = step.frames;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputInt("##rf", &frames)) {
          if (frames < 0) {
            frames = 0;
          }
          step.frames = frames;
          commit = true;
        }
      }
      ImGui::PopID();
    }
    if (ImGui::Button("Add step")) {
      RouteStep step;
      step.op = RouteStepOp::Move;
      step.dir = RampDirection::East;
      node.route.push_back(step);
      commit = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Del last") && !node.route.empty()) {
      node.route.pop_back();
      commit = true;
    }
  }
  return commit;
}

}  // namespace rat
