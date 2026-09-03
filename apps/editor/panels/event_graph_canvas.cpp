#include "event_graph_canvas.hpp"

#include "event_graph_edit.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_graph.hpp>
#include <rat/event_inspect.hpp>
#include <rat/map_document.hpp>

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace rat {
namespace {

constexpr float kNodeW = 156.0f;
constexpr float kNodeH = 54.0f;
constexpr float kPinR = 7.0f;

[[nodiscard]] const char* command_op_label(CommandOp op) {
  switch (op) {
    case CommandOp::ShowText:
      return "show_text";
    case CommandOp::ControlSwitch:
      return "control_switch";
    case CommandOp::ControlVariable:
      return "control_variable";
    case CommandOp::ControlSelfSwitch:
      return "control_self_switch";
    case CommandOp::ConditionalBranch:
      return "conditional_branch";
    case CommandOp::Wait:
      return "wait";
    case CommandOp::TransferPlayer:
      return "transfer_player";
    case CommandOp::ChangeItems:
      return "change_items";
    case CommandOp::PlaySE:
      return "play_se";
    case CommandOp::SetMoveRoute:
      return "set_move_route";
    case CommandOp::Comment:
      return "comment";
  }
  return "command";
}

[[nodiscard]] ImVec2 stored_pos(EventGraphCanvasState& canvas, const std::string& id, float x,
                                 float y) {
  auto found = canvas.pos.find(id);
  if (found == canvas.pos.end()) {
    canvas.pos[id] = {x, y};
    found = canvas.pos.find(id);
  }
  return ImVec2(found->second.first, found->second.second);
}

void set_stored_pos(EventGraphCanvasState& canvas, const std::string& id, ImVec2 pos) {
  canvas.pos[id] = {pos.x, pos.y};
}

[[nodiscard]] ImVec2 graph_to_screen(const ImVec2& origin, const EventGraphCanvasState& canvas,
                                     ImVec2 stored) {
  return ImVec2(origin.x + (stored.x + canvas.pan_x) * canvas.zoom,
                origin.y + (stored.y + canvas.pan_y) * canvas.zoom);
}

[[nodiscard]] float canvas_zoom(const EventGraphCanvasState& canvas) {
  return canvas.zoom <= 0.0f ? 1.0f : canvas.zoom;
}

[[nodiscard]] ImU32 kind_color(std::string_view kind) {
  if (kind == "show_text") {
    return IM_COL32(70, 110, 180, 255);
  }
  if (kind == "wait") {
    return IM_COL32(120, 80, 160, 255);
  }
  if (kind == "control_switch") {
    return IM_COL32(170, 110, 50, 255);
  }
  if (kind == "control_variable") {
    return IM_COL32(40, 130, 150, 255);
  }
  if (kind == "control_self_switch") {
    return IM_COL32(190, 90, 60, 255);
  }
  if (kind == "conditional_branch") {
    return IM_COL32(60, 140, 90, 255);
  }
  if (kind == "transfer_player") {
    return IM_COL32(50, 90, 160, 255);
  }
  if (kind == "change_items") {
    return IM_COL32(160, 130, 40, 255);
  }
  if (kind == "play_se") {
    return IM_COL32(150, 60, 140, 255);
  }
  if (kind == "set_move_route") {
    return IM_COL32(110, 80, 50, 255);
  }
  if (kind == "comment") {
    return IM_COL32(90, 90, 90, 255);
  }
  return IM_COL32(90, 90, 90, 255);
}

[[nodiscard]] std::string node_caption(const EventGraphNode& node) {
  if (node.kind == "show_text" || node.kind == "comment" || node.kind == "play_se") {
    return node.text.empty() ? node.kind : node.text;
  }
  if (node.kind == "wait") {
    return "wait " + std::to_string(node.frames);
  }
  if (node.kind == "control_switch") {
    return std::string("SW") + std::to_string(node.switch_id) + (node.bool_value ? " ON" : " OFF");
  }
  if (node.kind == "control_variable") {
    return std::string("VAR") + std::to_string(node.switch_id) + "=" + std::to_string(node.int_value);
  }
  if (node.kind == "control_self_switch") {
    return std::string("SS ") + node.self_switch + (node.bool_value ? " ON" : " OFF");
  }
  if (node.kind == "conditional_branch") {
    return summarize_condition(node.branch_condition);
  }
  if (node.kind == "transfer_player") {
    return node.map_id.empty() ? std::string("transfer") : node.map_id;
  }
  if (node.kind == "change_items") {
    return node.item_id.empty() ? std::string("items") : node.item_id;
  }
  if (node.kind == "set_move_route") {
    return std::string("route ") + std::to_string(node.route.size());
  }
  return node.kind;
}

void commit_event(EditorDocument& document, EventDef event) {
  (void)document.execute(make_replace_event_command(
      static_cast<std::size_t>(document.selected_event()), std::move(event)));
}

[[nodiscard]] EventGraphNode* find_node(EventGraph& graph, std::string_view id) {
  for (EventGraphNode& node : graph.nodes) {
    if (node.id == id) {
      return &node;
    }
  }
  return nullptr;
}

void draw_pin(ImDrawList* dl, ImVec2 center, bool hot, float radius) {
  dl->AddCircleFilled(center, radius, hot ? IM_COL32(240, 220, 90, 255) : IM_COL32(220, 220, 220, 255));
  dl->AddCircle(center, radius, IM_COL32(20, 20, 20, 255));
}

bool pin_hit(const char* id, ImVec2 center, float radius) {
  ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
  return ImGui::InvisibleButton(id, ImVec2(radius * 2.0f, radius * 2.0f));
}

void add_kind_node(EditorDocument& document, EventDef& event, EventGraphCanvasState& canvas,
                   std::string_view kind) {
  EventPage& page = event.pages[static_cast<std::size_t>(document.selected_page())];
  ensure_event_page_graph(page);
  EventGraph& graph = *page.graph;
  const std::string id = add_event_graph_node(graph, kind);
  if (id.empty()) {
    return;
  }
  if (graph.nodes.size() == 1 && graph.edges.empty()) {
    (void)connect_event_graph_nodes(graph, kEventGraphEntryId, id);
    (void)connect_event_graph_nodes(graph, id, kEventGraphExitId);
  }
  const float slot = static_cast<float>(graph.nodes.size());
  set_stored_pos(canvas, id, ImVec2(180.0f, 24.0f + (slot - 1.0f) * 70.0f));
  canvas.selected_id = id;
  commit_event(document, event);
}

void reload_selected_event(EditorDocument& document, EventDef& event) {
  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.visible_data().events.size())) {
    return;
  }
  event = document.visible_data().events[static_cast<std::size_t>(document.selected_event())];
}

}  // namespace

void draw_event_graph_canvas(EditorDocument& document, EventGraphCanvasState& canvas,
                             EventDef& event, std::string& compile_error) {
  if (canvas.bound_event != document.selected_event() ||
      canvas.bound_page != document.selected_page()) {
    canvas = EventGraphCanvasState{};
    canvas.bound_event = document.selected_event();
    canvas.bound_page = document.selected_page();
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Command graph");
  if (ImGui::Button("Show Text")) {
    add_kind_node(document, event, canvas, "show_text");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Switch")) {
    add_kind_node(document, event, canvas, "control_switch");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Variable")) {
    add_kind_node(document, event, canvas, "control_variable");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Self Switch")) {
    add_kind_node(document, event, canvas, "control_self_switch");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Branch")) {
    add_kind_node(document, event, canvas, "conditional_branch");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Wait")) {
    add_kind_node(document, event, canvas, "wait");
    reload_selected_event(document, event);
  }
  if (ImGui::Button("Transfer")) {
    add_kind_node(document, event, canvas, "transfer_player");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Items")) {
    add_kind_node(document, event, canvas, "change_items");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Play SE")) {
    add_kind_node(document, event, canvas, "play_se");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Move Route")) {
    add_kind_node(document, event, canvas, "set_move_route");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Comment")) {
    add_kind_node(document, event, canvas, "comment");
    reload_selected_event(document, event);
  }

  EventPage& page_now = event.pages[static_cast<std::size_t>(document.selected_page())];
  if (!page_now.graph.has_value()) {
    ImGui::TextWrapped("No graph: inspector list below is the command source. Add a node to start.");
    return;
  }

  EventGraph& graph = *page_now.graph;
  ImGui::TextWrapped("Click an out pin, then an in pin to connect. Apply/Save compiles to commands.");
  if (!canvas.pending_from.empty()) {
    const std::string pending_label =
        canvas.pending_branch.has_value() ? (" " + *canvas.pending_branch) : std::string();
    ImGui::Text("Connecting from %s%s", canvas.pending_from.c_str(), pending_label.c_str());
  }

  const float remain = ImGui::GetContentRegionAvail().y;
  constexpr float kReserveBelow = 200.0f;
  float canvas_h = remain - kReserveBelow;
  if (canvas_h < 400.0f) {
    canvas_h = 400.0f;
  }
  ImGui::BeginChild("event_graph_canvas", ImVec2(-1.0f, canvas_h), true,
                    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, canvas_h - 16.0f));
  const float zoom = canvas_zoom(canvas);
  const float node_w = kNodeW * zoom;
  const float node_h = kNodeH * zoom;
  const float pin_r = kPinR * zoom;
  const float rounding = 6.0f * zoom;

  struct NodeRect {
    std::string id;
    ImVec2 stored;
    bool branch = false;
  };
  std::vector<NodeRect> rects;
  rects.push_back(NodeRect{kEventGraphEntryId, stored_pos(canvas, kEventGraphEntryId, 12.0f, 110.0f),
                           false});
  rects.push_back(
      NodeRect{kEventGraphExitId, stored_pos(canvas, kEventGraphExitId, 360.0f, 110.0f), false});
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const EventGraphNode& node = graph.nodes[i];
    const float y = 20.0f + static_cast<float>(i) * 70.0f;
    rects.push_back(NodeRect{node.id, stored_pos(canvas, node.id, 180.0f, y),
                             node.kind == "conditional_branch"});
  }

  const auto rect_for = [&](const std::string& id) -> const NodeRect* {
    for (const NodeRect& rect : rects) {
      if (rect.id == id) {
        return &rect;
      }
    }
    return nullptr;
  };
  const auto screen_of = [&](ImVec2 stored) { return graph_to_screen(origin, canvas, stored); };

  for (const EventGraphEdge& edge : graph.edges) {
    const NodeRect* from = rect_for(edge.from);
    const NodeRect* to = rect_for(edge.to);
    if (from == nullptr || to == nullptr) {
      continue;
    }
    ImVec2 a_stored(from->stored.x + kNodeW, from->stored.y + kNodeH * 0.5f);
    if (from->branch && edge.branch.has_value() && *edge.branch == "else") {
      a_stored.y = from->stored.y + kNodeH - 12.0f;
    } else if (from->branch && edge.branch.has_value() && *edge.branch == "then") {
      a_stored.y = from->stored.y + 12.0f;
    }
    const ImVec2 a = screen_of(a_stored);
    const ImVec2 b = screen_of(ImVec2(to->stored.x, to->stored.y + kNodeH * 0.5f));
    const bool selected = canvas.selected_edge_from == edge.from && canvas.selected_edge_to == edge.to;
    const float handle = 40.0f * zoom;
    dl->AddBezierCubic(a, ImVec2(a.x + handle, a.y), ImVec2(b.x - handle, b.y), b,
                       selected ? IM_COL32(255, 210, 80, 255) : IM_COL32(200, 200, 200, 220),
                       std::max(1.0f, 2.0f * zoom));
  }

  std::string connect_from;
  std::string connect_to;
  std::optional<std::string> connect_branch;
  bool do_connect = false;
  bool dragging_node = false;

  const auto handle_out_pin = [&](const std::string& id, ImVec2 center, std::optional<std::string> branch,
                                  const char* ui_id) {
    draw_pin(dl, center, canvas.pending_from == id && canvas.pending_branch == branch, pin_r);
    if (pin_hit(ui_id, center, pin_r)) {
      canvas.pending_from = id;
      canvas.pending_branch = std::move(branch);
    }
  };
  const auto handle_in_pin = [&](const std::string& id, ImVec2 center, const char* ui_id) {
    draw_pin(dl, center, false, pin_r);
    if (pin_hit(ui_id, center, pin_r) && !canvas.pending_from.empty()) {
      connect_from = canvas.pending_from;
      connect_to = id;
      connect_branch = canvas.pending_branch;
      do_connect = true;
    }
  };

  for (const NodeRect& rect : rects) {
    const ImVec2 pos = screen_of(rect.stored);
    const bool selected = canvas.selected_id == rect.id;
    std::string kind = rect.id;
    std::string caption = rect.id;
    if (rect.id == kEventGraphEntryId) {
      kind = "entry";
      caption = "entry";
    } else if (rect.id == kEventGraphExitId) {
      kind = "exit";
      caption = "exit";
    } else if (const EventGraphNode* node = find_node(graph, rect.id)) {
      kind = node->kind;
      caption = node_caption(*node);
    }
    dl->AddRectFilled(pos, ImVec2(pos.x + node_w, pos.y + node_h), kind_color(kind), rounding);
    dl->AddRect(pos, ImVec2(pos.x + node_w, pos.y + node_h),
                selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(20, 20, 20, 255), rounding, 0,
                selected ? 2.0f : 1.0f);
    dl->AddText(ImVec2(pos.x + 10.0f * zoom, pos.y + 8.0f * zoom), IM_COL32(255, 255, 255, 255),
                kind.c_str());
    dl->AddText(ImVec2(pos.x + 10.0f * zoom, pos.y + 26.0f * zoom), IM_COL32(230, 230, 230, 255),
                caption.c_str());

    ImGui::SetCursorScreenPos(pos);
    const std::string hit_id = std::string("##node_") + rect.id;
    ImGui::InvisibleButton(hit_id.c_str(), ImVec2(node_w, node_h));
    if (ImGui::IsItemClicked()) {
      canvas.selected_id = rect.id;
      canvas.selected_edge_from.clear();
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      dragging_node = true;
      const ImVec2 delta = ImGui::GetIO().MouseDelta;
      set_stored_pos(canvas, rect.id,
                     ImVec2(rect.stored.x + delta.x / zoom, rect.stored.y + delta.y / zoom));
    }

    if (rect.id != kEventGraphExitId) {
      if (rect.branch) {
        handle_out_pin(rect.id, screen_of(ImVec2(rect.stored.x + kNodeW, rect.stored.y + 12.0f)),
                       std::string("then"), (std::string("##out_then_") + rect.id).c_str());
        handle_out_pin(rect.id,
                       screen_of(ImVec2(rect.stored.x + kNodeW, rect.stored.y + kNodeH - 12.0f)),
                       std::string("else"), (std::string("##out_else_") + rect.id).c_str());
      } else {
        handle_out_pin(rect.id,
                       screen_of(ImVec2(rect.stored.x + kNodeW, rect.stored.y + kNodeH * 0.5f)),
                       std::nullopt, (std::string("##out_") + rect.id).c_str());
      }
    }
    if (rect.id != kEventGraphEntryId) {
      handle_in_pin(rect.id, screen_of(ImVec2(rect.stored.x, rect.stored.y + kNodeH * 0.5f)),
                    (std::string("##in_") + rect.id).c_str());
    }
  }

  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    canvas.pending_from.clear();
    canvas.pending_branch.reset();
  }

  const ImGuiIO& io = ImGui::GetIO();
  if (ImGui::IsWindowHovered()) {
    if (io.MouseWheel != 0.0f) {
      const float old_zoom = zoom;
      float new_zoom = old_zoom * (io.MouseWheel > 0.0f ? 1.1f : 1.0f / 1.1f);
      new_zoom = std::clamp(new_zoom, 0.25f, 3.0f);
      const ImVec2 local(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
      canvas.pan_x += local.x / new_zoom - local.x / old_zoom;
      canvas.pan_y += local.y / new_zoom - local.y / old_zoom;
      canvas.zoom = new_zoom;
    }
    const bool pan_mmb = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
    const bool pan_alt =
        io.KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !dragging_node;
    if (pan_mmb || pan_alt) {
      canvas.pan_x += io.MouseDelta.x / canvas_zoom(canvas);
      canvas.pan_y += io.MouseDelta.y / canvas_zoom(canvas);
    }
  }
  ImGui::EndChild();

  if (do_connect) {
    if (connect_event_graph_nodes(graph, connect_from, connect_to, connect_branch)) {
      canvas.selected_edge_from = connect_from;
      canvas.selected_edge_to = connect_to;
      canvas.selected_edge_branch = connect_branch;
      canvas.pending_from.clear();
      canvas.pending_branch.reset();
      commit_event(document, event);
      reload_selected_event(document, event);
    }
  }

  EventPage& page_after = event.pages[static_cast<std::size_t>(document.selected_page())];
  EventGraph* live_graph = page_after.graph ? &*page_after.graph : nullptr;
  if (live_graph == nullptr) {
    return;
  }

  if (ImGui::BeginListBox("##graph_edges", ImVec2(-1.0f, 72.0f))) {
    for (std::size_t i = 0; i < live_graph->edges.size(); ++i) {
      const EventGraphEdge& edge = live_graph->edges[i];
      char label[192];
      if (edge.branch.has_value() && !edge.branch->empty()) {
        std::snprintf(label, sizeof(label), "%s -%s-> %s", edge.from.c_str(), edge.branch->c_str(),
                      edge.to.c_str());
      } else {
        std::snprintf(label, sizeof(label), "%s -> %s", edge.from.c_str(), edge.to.c_str());
      }
      const bool selected = canvas.selected_edge_from == edge.from && canvas.selected_edge_to == edge.to &&
                            canvas.selected_edge_branch == edge.branch;
      if (ImGui::Selectable(label, selected)) {
        canvas.selected_edge_from = edge.from;
        canvas.selected_edge_to = edge.to;
        canvas.selected_edge_branch = edge.branch;
      }
      (void)i;
    }
    ImGui::EndListBox();
  }

  if (ImGui::Button("Delete node") && !canvas.selected_id.empty() &&
      canvas.selected_id != kEventGraphEntryId && canvas.selected_id != kEventGraphExitId) {
    if (delete_event_graph_node(*live_graph, canvas.selected_id)) {
      canvas.selected_id.clear();
      commit_event(document, event);
      reload_selected_event(document, event);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete edge") && !canvas.selected_edge_from.empty()) {
    EventPage& page_del = event.pages[static_cast<std::size_t>(document.selected_page())];
    if (page_del.graph.has_value() &&
        delete_event_graph_edge(*page_del.graph, canvas.selected_edge_from, canvas.selected_edge_to,
                                canvas.selected_edge_branch)) {
      canvas.selected_edge_from.clear();
      canvas.selected_edge_to.clear();
      canvas.selected_edge_branch.reset();
      commit_event(document, event);
      reload_selected_event(document, event);
    }
  }

  EventPage& page_sel = event.pages[static_cast<std::size_t>(document.selected_page())];
  EventGraphNode* selected =
      page_sel.graph.has_value() ? find_node(*page_sel.graph, canvas.selected_id) : nullptr;
  if (selected != nullptr) {
    ImGui::Separator();
    ImGui::Text("Node %s (%s)", selected->id.c_str(), selected->kind.c_str());
    if (selected->kind == "show_text" || selected->kind == "comment") {
      char buf[512];
      std::snprintf(buf, sizeof(buf), "%s", selected->text.c_str());
      const char* label = selected->kind == "comment" ? "Comment" : "Text";
      if (ImGui::InputTextMultiline(label, buf, sizeof(buf), ImVec2(-1.0f, 50.0f))) {
        selected->text = buf;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        commit_event(document, event);
      }
    } else if (selected->kind == "play_se") {
      char buf[256];
      std::snprintf(buf, sizeof(buf), "%s", selected->text.c_str());
      if (ImGui::InputText("Clip id", buf, sizeof(buf))) {
        selected->text = buf;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        commit_event(document, event);
      }
    } else if (selected->kind == "wait") {
      int frames = selected->frames;
      if (ImGui::InputInt("Frames", &frames)) {
        if (frames < 0) {
          frames = 0;
        }
        selected->frames = frames;
        commit_event(document, event);
      }
    } else if (selected->kind == "control_switch") {
      int switch_id = static_cast<int>(selected->switch_id);
      if (ImGui::InputInt("Switch id", &switch_id)) {
        if (switch_id < 0) {
          switch_id = 0;
        }
        selected->switch_id = static_cast<std::uint32_t>(switch_id);
        commit_event(document, event);
      }
      bool on = selected->bool_value;
      if (ImGui::Checkbox("ON", &on)) {
        selected->bool_value = on;
        commit_event(document, event);
      }
    } else if (selected->kind == "control_variable") {
      int var_id = static_cast<int>(selected->switch_id);
      if (ImGui::InputInt("Variable id", &var_id)) {
        if (var_id < 0) {
          var_id = 0;
        }
        selected->switch_id = static_cast<std::uint32_t>(var_id);
        commit_event(document, event);
      }
      int value = selected->int_value;
      if (ImGui::InputInt("Value", &value)) {
        selected->int_value = value;
        commit_event(document, event);
      }
    } else if (selected->kind == "control_self_switch") {
      int ss = selected->self_switch - 'A';
      if (ss < 0 || ss > 3) {
        ss = 0;
      }
      if (ImGui::Combo("Self switch", &ss, "A\0B\0C\0D\0")) {
        selected->self_switch = static_cast<char>('A' + ss);
        commit_event(document, event);
      }
      bool on = selected->bool_value;
      if (ImGui::Checkbox("ON", &on)) {
        selected->bool_value = on;
        commit_event(document, event);
      }
    } else if (selected->kind == "conditional_branch") {
      int switch_id = static_cast<int>(selected->branch_condition.id);
      if (ImGui::InputInt("If SW id", &switch_id)) {
        if (switch_id < 0) {
          switch_id = 0;
        }
        selected->branch_condition.type = ConditionType::Switch;
        selected->branch_condition.id = static_cast<std::uint32_t>(switch_id);
        commit_event(document, event);
      }
      bool on = selected->branch_condition.bool_value;
      if (ImGui::Checkbox("If SW ON", &on)) {
        selected->branch_condition.bool_value = on;
        commit_event(document, event);
      }
    } else if (selected->kind == "transfer_player") {
      char buf[128];
      std::snprintf(buf, sizeof(buf), "%s", selected->map_id.c_str());
      if (ImGui::InputText("Map id", buf, sizeof(buf))) {
        selected->map_id = buf;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        commit_event(document, event);
      }
      float x = selected->x;
      float y = selected->y;
      float z = selected->z;
      if (ImGui::InputFloat("X", &x)) {
        selected->x = x;
        commit_event(document, event);
      }
      if (ImGui::InputFloat("Y", &y)) {
        selected->y = y;
        commit_event(document, event);
      }
      if (ImGui::InputFloat("Z", &z)) {
        selected->z = z;
        commit_event(document, event);
      }
    } else if (selected->kind == "change_items") {
      char buf[128];
      std::snprintf(buf, sizeof(buf), "%s", selected->item_id.c_str());
      if (ImGui::InputText("Item id", buf, sizeof(buf))) {
        selected->item_id = buf;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        commit_event(document, event);
      }
      int delta = selected->item_delta;
      if (ImGui::InputInt("Delta", &delta)) {
        selected->item_delta = delta;
        commit_event(document, event);
      }
      bool key = selected->key_item;
      if (ImGui::Checkbox("Key item", &key)) {
        selected->key_item = key;
        commit_event(document, event);
      }
    } else if (selected->kind == "set_move_route") {
      bool through = selected->through;
      if (ImGui::Checkbox("Through", &through)) {
        selected->through = through;
        commit_event(document, event);
      }
      for (std::size_t i = 0; i < selected->route.size(); ++i) {
        RouteStep& step = selected->route[i];
        ImGui::PushID(static_cast<int>(i));
        int op = static_cast<int>(step.op);
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::Combo("Op", &op, "Move\0Wait\0Turn\0")) {
          step.op = static_cast<RouteStepOp>(op);
          commit_event(document, event);
        }
        if (step.op == RouteStepOp::Move || step.op == RouteStepOp::Turn) {
          int dir = static_cast<int>(step.dir);
          ImGui::SameLine();
          ImGui::SetNextItemWidth(90.0f);
          if (ImGui::Combo("Dir", &dir, "North\0East\0South\0West\0")) {
            step.dir = static_cast<RampDirection>(dir);
            commit_event(document, event);
          }
        } else if (step.op == RouteStepOp::Wait) {
          int frames = step.frames;
          ImGui::SameLine();
          ImGui::SetNextItemWidth(80.0f);
          if (ImGui::InputInt("Frames", &frames)) {
            if (frames < 0) {
              frames = 0;
            }
            step.frames = frames;
            commit_event(document, event);
          }
        }
        ImGui::PopID();
      }
      if (ImGui::Button("Add step")) {
        RouteStep step;
        step.op = RouteStepOp::Move;
        step.dir = RampDirection::East;
        selected->route.push_back(step);
        commit_event(document, event);
      }
      ImGui::SameLine();
      if (ImGui::Button("Remove last") && !selected->route.empty()) {
        selected->route.pop_back();
        commit_event(document, event);
      }
    }
  }

  if (ImGui::Button("Compile graph")) {
    const EventGraphApplyResult compiled = document.compile_graphs_for_apply();
    if (compiled.ok) {
      compile_error.clear();
      reload_selected_event(document, event);
    } else {
      compile_error = compiled.issues.empty() ? "event graph compile failed"
                                             : format_map_issues(compiled.issues);
    }
  }
  if (!compile_error.empty()) {
    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", compile_error.c_str());
  }

  if (document.selected_event() < 0 ||
      document.selected_event() >= static_cast<int>(document.data().events.size())) {
    return;
  }
  const EventPage& compiled_page =
      document.data().events[static_cast<std::size_t>(document.selected_event())]
          .pages[static_cast<std::size_t>(document.selected_page())];
  ImGui::TextUnformatted("Compiled commands (Play uses this after apply):");
  if (compiled_page.commands.empty()) {
    ImGui::TextUnformatted("(empty until compile/apply)");
  } else {
    for (std::size_t i = 0; i < compiled_page.commands.size(); ++i) {
      const Command& cmd = compiled_page.commands[i];
      ImGui::Text("%zu. %s %s", i, command_op_label(cmd.op), cmd.text.c_str());
    }
  }
}

}  // namespace rat
