#pragma once

#include <rat/map_data.hpp>

namespace rat {

// ImGui controls for one graph node body (kind is already in the title). True = commit.
bool draw_event_graph_node_fields(EventGraphNode& node, float zoom);

}  // namespace rat
