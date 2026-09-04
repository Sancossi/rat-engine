#pragma once

#include <rat/map_data.hpp>

namespace rat {

struct EventGraphNodeFieldResult {
  bool preview = false;
  bool commit = false;
};

// ImGui controls for one graph node body (kind is already in the title).
EventGraphNodeFieldResult draw_event_graph_node_fields(EventGraphNode& node, float zoom);

}  // namespace rat
