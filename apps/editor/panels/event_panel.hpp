#pragma once

#include "editor_document.hpp"

#include <optional>
#include <string>

namespace rat {

struct EventPanelState {
  std::optional<EventDef> field_origin;
  int field_origin_index = -1;
  int next_stub_event = 1;
};

void draw_event_panel(EditorDocument& document, EventPanelState& state, const char* why_not);

}  // namespace rat
