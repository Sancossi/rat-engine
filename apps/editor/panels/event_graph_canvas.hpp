#pragma once

#include "editor_document.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace rat {

struct EventGraphCanvasState {
  int bound_event = -1;
  int bound_page = -1;
  std::string selected_id;
  std::string pending_from;
  std::optional<std::string> pending_branch;
  std::string selected_edge_from;
  std::string selected_edge_to;
  std::optional<std::string> selected_edge_branch;
  std::unordered_map<std::string, std::pair<float, float>> pos;
  float pan_x = 0.0f;
  float pan_y = 0.0f;
  float zoom = 1.0f;
};

void draw_event_graph_canvas(EditorDocument& document, EventGraphCanvasState& canvas,
                             EventDef& event, std::string& compile_error);

}  // namespace rat
