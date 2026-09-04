#include "event_graph_canvas.hpp"

#include "event_graph_edit.hpp"
#include "event_graph_node_ui.hpp"

#include <rat/edit_history.hpp>
#include <rat/event_graph.hpp>
#include <rat/map_document.hpp>

#include <imgui.h>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rat {
namespace {

constexpr float kPinR = 7.0f;
constexpr float kTitleDragH = 22.0f;

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

void commit_event(EditorDocument& document, EventDef event) {
  (void)document.execute(make_replace_event_command(
      static_cast<std::size_t>(document.selected_event()), std::move(event)));
}

void preview_event(EditorDocument& document, EventDef event) {
  (void)document.preview_event(document.selected_event(), std::move(event));
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

[[nodiscard]] ImVec2 bezier_point(ImVec2 a, ImVec2 c1, ImVec2 c2, ImVec2 b, float t) {
  const float u = 1.0f - t;
  const float uu = u * u;
  const float tt = t * t;
  const float uuu = uu * u;
  const float ttt = tt * t;
  return ImVec2(uuu * a.x + 3.0f * uu * t * c1.x + 3.0f * u * tt * c2.x + ttt * b.x,
                uuu * a.y + 3.0f * uu * t * c1.y + 3.0f * u * tt * c2.y + ttt * b.y);
}

[[nodiscard]] bool near_bezier(ImVec2 a, ImVec2 c1, ImVec2 c2, ImVec2 b, ImVec2 p, float dist) {
  const float dist2 = dist * dist;
  for (int i = 0; i <= 16; ++i) {
    const ImVec2 pt = bezier_point(a, c1, c2, b, static_cast<float>(i) / 16.0f);
    const float dx = pt.x - p.x;
    const float dy = pt.y - p.y;
    if (dx * dx + dy * dy <= dist2) {
      return true;
    }
  }
  return false;
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
  const EventGraphNodeMetrics metrics = event_graph_node_metrics(kind);
  const float slot = static_cast<float>(graph.nodes.size());
  set_stored_pos(canvas, id, ImVec2(180.0f, 24.0f + (slot - 1.0f) * (metrics.height + 16.0f)));
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
    ImGui::TextWrapped("No graph yet. Add a node to start. Play runs the graph.");
    return;
  }

  EventGraph& graph = *page_now.graph;
  ImGui::TextWrapped("Click an out pin, then an in pin to connect. Play runs this graph.");
  if (!canvas.pending_from.empty()) {
    const std::string pending_label =
        canvas.pending_branch.has_value() ? (" " + *canvas.pending_branch) : std::string();
    ImGui::Text("Connecting from %s%s", canvas.pending_from.c_str(), pending_label.c_str());
  }

  const float remain = ImGui::GetContentRegionAvail().y;
  constexpr float kReserveBelow = 80.0f;
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
  const float pin_r = kPinR * zoom;
  const float rounding = 6.0f * zoom;

  struct NodeRect {
    std::string id;
    ImVec2 stored;
    EventGraphNodeMetrics metrics;
    bool branch = false;
  };
  std::vector<NodeRect> rects;
  rects.push_back(NodeRect{kEventGraphEntryId, stored_pos(canvas, kEventGraphEntryId, 12.0f, 110.0f),
                           event_graph_node_metrics("entry"), false});
  rects.push_back(NodeRect{kEventGraphExitId, stored_pos(canvas, kEventGraphExitId, 420.0f, 110.0f),
                           event_graph_node_metrics("exit"), false});
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const EventGraphNode& node = graph.nodes[i];
    const float y = 20.0f + static_cast<float>(i) * 120.0f;
    rects.push_back(NodeRect{node.id, stored_pos(canvas, node.id, 180.0f, y),
                             event_graph_node_metrics(node.kind, node.route.size()),
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

  struct EdgeGeom {
    EventGraphEdge edge;
    ImVec2 a;
    ImVec2 c1;
    ImVec2 c2;
    ImVec2 b;
  };
  std::vector<EdgeGeom> edge_geoms;
  for (const EventGraphEdge& edge : graph.edges) {
    const NodeRect* from = rect_for(edge.from);
    const NodeRect* to = rect_for(edge.to);
    if (from == nullptr || to == nullptr) {
      continue;
    }
    ImVec2 a_stored(from->stored.x + from->metrics.out_pin_x,
                    from->stored.y + from->metrics.seq_out_pin_y);
    if (from->branch && edge.branch.has_value() && *edge.branch == "else") {
      a_stored.y = from->stored.y + from->metrics.else_pin_y;
    } else if (from->branch && edge.branch.has_value() && *edge.branch == "then") {
      a_stored.y = from->stored.y + from->metrics.then_pin_y;
    }
    const ImVec2 a = screen_of(a_stored);
    const ImVec2 b =
        screen_of(ImVec2(to->stored.x + to->metrics.in_pin_x, to->stored.y + to->metrics.in_pin_y));
    const float handle = 40.0f * zoom;
    const ImVec2 c1(a.x + handle, a.y);
    const ImVec2 c2(b.x - handle, b.y);
    const bool selected = canvas.selected_edge_from == edge.from && canvas.selected_edge_to == edge.to;
    dl->AddBezierCubic(a, c1, c2, b,
                       selected ? IM_COL32(255, 210, 80, 255) : IM_COL32(200, 200, 200, 220),
                       std::max(1.0f, 2.0f * zoom));
    edge_geoms.push_back(EdgeGeom{edge, a, c1, c2, b});
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

  bool mouse_over_node = false;
  const ImVec2 mouse = ImGui::GetIO().MousePos;
  for (const NodeRect& rect : rects) {
    const ImVec2 pos = screen_of(rect.stored);
    const float node_w = rect.metrics.width * zoom;
    const float node_h = rect.metrics.height * zoom;
    if (mouse.x >= pos.x && mouse.x <= pos.x + node_w && mouse.y >= pos.y &&
        mouse.y <= pos.y + node_h) {
      mouse_over_node = true;
    }
    const bool selected = canvas.selected_id == rect.id;
    std::string kind = rect.id;
    if (rect.id == kEventGraphEntryId) {
      kind = "entry";
    } else if (rect.id == kEventGraphExitId) {
      kind = "exit";
    } else if (const EventGraphNode* node = find_node(graph, rect.id)) {
      kind = node->kind;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(kind_color(kind)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);
    ImGui::SetCursorScreenPos(pos);
    const std::string body_id = std::string("##body_") + rect.id;
    ImGui::BeginChild(body_id.c_str(), ImVec2(node_w, node_h), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                          ImGuiWindowFlags_NoMove);
    ImGui::SetWindowFontScale(zoom);
    ImGui::SetCursorPos(ImVec2(8.0f * zoom, 4.0f * zoom));
    ImGui::TextUnformatted(kind.c_str());
    ImGui::SetCursorScreenPos(pos);
    const std::string drag_id = std::string("##drag_") + rect.id;
    ImGui::InvisibleButton(drag_id.c_str(), ImVec2(node_w, kTitleDragH * zoom));
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
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      canvas.selected_id = rect.id;
      canvas.selected_edge_from.clear();
    }

    EventGraphNode* live = find_node(graph, rect.id);
    if (live != nullptr) {
      ImGui::SetCursorPos(ImVec2(8.0f * zoom, (rect.metrics.title_h + 8.0f) * zoom));
      ImGui::PushItemWidth((rect.metrics.width - 16.0f) * zoom);
      const EventGraphNodeFieldResult fields = draw_event_graph_node_fields(*live, zoom);
      if (fields.preview) {
        preview_event(document, event);
      }
      if (fields.commit) {
        commit_event(document, event);
      }
      ImGui::PopItemWidth();
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    dl->AddRect(pos, ImVec2(pos.x + node_w, pos.y + node_h),
                selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(20, 20, 20, 255), rounding, 0,
                selected ? 2.0f : 1.0f);

    if (rect.id != kEventGraphExitId) {
      if (rect.branch) {
        handle_out_pin(rect.id,
                       screen_of(ImVec2(rect.stored.x + rect.metrics.out_pin_x,
                                        rect.stored.y + rect.metrics.then_pin_y)),
                       std::string("then"), (std::string("##out_then_") + rect.id).c_str());
        handle_out_pin(rect.id,
                       screen_of(ImVec2(rect.stored.x + rect.metrics.out_pin_x,
                                        rect.stored.y + rect.metrics.else_pin_y)),
                       std::string("else"), (std::string("##out_else_") + rect.id).c_str());
      } else {
        handle_out_pin(rect.id,
                       screen_of(ImVec2(rect.stored.x + rect.metrics.out_pin_x,
                                        rect.stored.y + rect.metrics.seq_out_pin_y)),
                       std::nullopt, (std::string("##out_") + rect.id).c_str());
      }
    }
    if (rect.id != kEventGraphEntryId) {
      handle_in_pin(rect.id,
                    screen_of(ImVec2(rect.stored.x + rect.metrics.in_pin_x,
                                     rect.stored.y + rect.metrics.in_pin_y)),
                    (std::string("##in_") + rect.id).c_str());
    }
  }

  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
      ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    canvas.pending_from.clear();
    canvas.pending_branch.reset();
  }

  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mouse_over_node &&
      !dragging_node) {
    bool hit_edge = false;
    for (const EdgeGeom& geom : edge_geoms) {
      if (near_bezier(geom.a, geom.c1, geom.c2, geom.b, mouse, 8.0f * zoom)) {
        canvas.selected_edge_from = geom.edge.from;
        canvas.selected_edge_to = geom.edge.to;
        canvas.selected_edge_branch = geom.edge.branch;
        canvas.selected_id.clear();
        hit_edge = true;
        break;
      }
    }
    if (!hit_edge) {
      canvas.selected_edge_from.clear();
      canvas.selected_edge_to.clear();
      canvas.selected_edge_branch.reset();
    }
  }

  const ImGuiIO& io = ImGui::GetIO();
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
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
}

}  // namespace rat
