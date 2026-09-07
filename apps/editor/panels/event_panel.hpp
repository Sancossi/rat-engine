#pragma once

#include "editor_document.hpp"
#include "event_graph_canvas.hpp"

#include <optional>
#include <string>

namespace rat {

struct EventPanelState {
  std::optional<EventDef> field_origin;
  int field_origin_index = -1;
  int graph_tab_select = -1;
  EventGraphCanvasState canvas;
  std::string last_compile_error;
};

void draw_event_panel(EditorDocument& document, EventPanelState& state, const char* why_not,
                      bool* event_graph_open = nullptr);

}  // namespace rat
