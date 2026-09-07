#pragma once

#include "editor_document.hpp"

#include <rat/map_data.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace rat {

struct EventGraphCanvasState : EventGraphWireState {
  int bound_event = -1;
  int bound_page = -1;
  std::string selected_id;
  std::string selected_edge_from;
  std::string selected_edge_to;
  std::optional<std::string> selected_edge_branch;
  std::unordered_map<std::string, std::pair<float, float>> pos;
  float pan_x = 0.0f;
  float pan_y = 0.0f;
  float zoom = 1.0f;
  float display_scale = 1.0f;  // Transient UI scale; authored positions stay in graph units.
  std::optional<EventGraphNode> clipboard;
  float clipboard_x = 0.0f;
  float clipboard_y = 0.0f;
  std::optional<EventGraphNodeLayout> node_drag_initial_layout;
  float node_drag_initial_x = 0.0f;
  float node_drag_initial_y = 0.0f;
  float context_x = 180.0f;
  float context_y = 24.0f;
};

void draw_event_graph_canvas(EditorDocument& document, EventGraphCanvasState& canvas,
                             EventDef& event, std::string& compile_error);

}  // namespace rat
