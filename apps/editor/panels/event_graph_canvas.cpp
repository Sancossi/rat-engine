#include "event_graph_canvas.hpp"

#include "event_graph_edit.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_graph.hpp>
#include <rat/event_inspect.hpp>
#include <rat/map_document.hpp>

#include <imgui.h>

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
  if (kind == "conditional_branch") {
    return IM_COL32(60, 140, 90, 255);
  }
  return IM_COL32(90, 90, 90, 255);
}

[[nodiscard]] std::string node_caption(const EventGraphNode& node) {
  if (node.kind == "show_text") {
    return node.text.empty() ? std::string("show_text") : node.text;
  }
  if (node.kind == "wait") {
    return "wait " + std::to_string(node.frames);
  }
  if (node.kind == "control_switch") {
    return std::string("SW") + std::to_string(node.switch_id) + (node.bool_value ? " ON" : " OFF");
  }
  if (node.kind == "conditional_branch") {
    return summarize_condition(node.branch_condition);
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

void draw_pin(ImDrawList* dl, ImVec2 center, bool hot) {
  dl->AddCircleFilled(center, kPinR, hot ? IM_COL32(240, 220, 90, 255) : IM_COL32(220, 220, 220, 255));
  dl->AddCircle(center, kPinR, IM_COL32(20, 20, 20, 255));
}

bool pin_hit(const char* id, ImVec2 center) {
  ImGui::SetCursorScreenPos(ImVec2(center.x - kPinR, center.y - kPinR));
  return ImGui::InvisibleButton(id, ImVec2(kPinR * 2.0f, kPinR * 2.0f));
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
  ImGui::TextUnformatted("Command graph (MVP)");
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
  if (ImGui::Button("Branch")) {
    add_kind_node(document, event, canvas, "conditional_branch");
    reload_selected_event(document, event);
  }
  ImGui::SameLine();
  if (ImGui::Button("Wait")) {
    add_kind_node(document, event, canvas, "wait");
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

  ImGui::BeginChild("event_graph_canvas", ImVec2(-1.0f, 280.0f), true);
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  ImGui::Dummy(ImVec2(520.0f, 250.0f));

  struct NodeRect {
    std::string id;
    ImVec2 pos;
    bool branch = false;
  };
  std::vector<NodeRect> rects;
  rects.push_back(
      NodeRect{kEventGraphEntryId, origin + stored_pos(canvas, kEventGraphEntryId, 12.0f, 110.0f), false});
  rects.push_back(
      NodeRect{kEventGraphExitId, origin + stored_pos(canvas, kEventGraphExitId, 360.0f, 110.0f), false});
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const EventGraphNode& node = graph.nodes[i];
    const float y = 20.0f + static_cast<float>(i) * 70.0f;
    rects.push_back(
        NodeRect{node.id, origin + stored_pos(canvas, node.id, 180.0f, y),
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

  for (const EventGraphEdge& edge : graph.edges) {
    const NodeRect* from = rect_for(edge.from);
    const NodeRect* to = rect_for(edge.to);
    if (from == nullptr || to == nullptr) {
      continue;
    }
    ImVec2 a(from->pos.x + kNodeW, from->pos.y + kNodeH * 0.5f);
    if (from->branch && edge.branch.has_value() && *edge.branch == "else") {
      a.y = from->pos.y + kNodeH - 12.0f;
    } else if (from->branch && edge.branch.has_value() && *edge.branch == "then") {
      a.y = from->pos.y + 12.0f;
    }
    const ImVec2 b(to->pos.x, to->pos.y + kNodeH * 0.5f);
    const bool selected = canvas.selected_edge_from == edge.from && canvas.selected_edge_to == edge.to;
    dl->AddBezierCubic(a, ImVec2(a.x + 40.0f, a.y), ImVec2(b.x - 40.0f, b.y), b,
                       selected ? IM_COL32(255, 210, 80, 255) : IM_COL32(200, 200, 200, 220), 2.0f);
  }

  std::string connect_from;
  std::string connect_to;
  std::optional<std::string> connect_branch;
  bool do_connect = false;

  const auto handle_out_pin = [&](const std::string& id, ImVec2 center, std::optional<std::string> branch,
                                  const char* ui_id) {
    draw_pin(dl, center, canvas.pending_from == id && canvas.pending_branch == branch);
    if (pin_hit(ui_id, center)) {
      canvas.pending_from = id;
      canvas.pending_branch = std::move(branch);
    }
  };
  const auto handle_in_pin = [&](const std::string& id, ImVec2 center, const char* ui_id) {
    draw_pin(dl, center, false);
    if (pin_hit(ui_id, center) && !canvas.pending_from.empty()) {
      connect_from = canvas.pending_from;
      connect_to = id;
      connect_branch = canvas.pending_branch;
      do_connect = true;
    }
  };

  for (const NodeRect& rect : rects) {
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
    dl->AddRectFilled(rect.pos, ImVec2(rect.pos.x + kNodeW, rect.pos.y + kNodeH), kind_color(kind), 6.0f);
    dl->AddRect(rect.pos, ImVec2(rect.pos.x + kNodeW, rect.pos.y + kNodeH),
                selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(20, 20, 20, 255), 6.0f, 0,
                selected ? 2.0f : 1.0f);
    dl->AddText(ImVec2(rect.pos.x + 10.0f, rect.pos.y + 8.0f), IM_COL32(255, 255, 255, 255),
                kind.c_str());
    dl->AddText(ImVec2(rect.pos.x + 10.0f, rect.pos.y + 26.0f), IM_COL32(230, 230, 230, 255),
                caption.c_str());

    ImGui::SetCursorScreenPos(rect.pos);
    const std::string hit_id = std::string("##node_") + rect.id;
    ImGui::InvisibleButton(hit_id.c_str(), ImVec2(kNodeW, kNodeH));
    if (ImGui::IsItemClicked()) {
      canvas.selected_id = rect.id;
      canvas.selected_edge_from.clear();
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      const ImVec2 delta = ImGui::GetIO().MouseDelta;
      set_stored_pos(canvas, rect.id,
                     ImVec2(rect.pos.x - origin.x + delta.x, rect.pos.y - origin.y + delta.y));
    }

    if (rect.id != kEventGraphExitId) {
      if (rect.branch) {
        handle_out_pin(rect.id, ImVec2(rect.pos.x + kNodeW, rect.pos.y + 12.0f), std::string("then"),
                       (std::string("##out_then_") + rect.id).c_str());
        handle_out_pin(rect.id, ImVec2(rect.pos.x + kNodeW, rect.pos.y + kNodeH - 12.0f),
                       std::string("else"), (std::string("##out_else_") + rect.id).c_str());
      } else {
        handle_out_pin(rect.id, ImVec2(rect.pos.x + kNodeW, rect.pos.y + kNodeH * 0.5f), std::nullopt,
                       (std::string("##out_") + rect.id).c_str());
      }
    }
    if (rect.id != kEventGraphEntryId) {
      handle_in_pin(rect.id, ImVec2(rect.pos.x, rect.pos.y + kNodeH * 0.5f),
                    (std::string("##in_") + rect.id).c_str());
    }
  }

  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    canvas.pending_from.clear();
    canvas.pending_branch.reset();
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
    if (selected->kind == "show_text") {
      char buf[512];
      std::snprintf(buf, sizeof(buf), "%s", selected->text.c_str());
      if (ImGui::InputTextMultiline("Text", buf, sizeof(buf), ImVec2(-1.0f, 50.0f))) {
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
