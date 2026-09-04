#pragma once

#include "rat/map_data.hpp"
#include "rat/map_document.hpp"

#include <vector>

namespace rat {

struct EventGraphCompileResult {
  bool ok = false;
  std::vector<Command> commands;
  std::vector<MapIssue> issues;
};

[[nodiscard]] EventGraphCompileResult compile_event_graph(const EventGraph& graph);

[[nodiscard]] EventGraph commands_to_graph(const std::vector<Command>& commands);
void ensure_page_graph_from_commands(EventPage& page);

}  // namespace rat
