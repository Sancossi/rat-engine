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

}  // namespace rat
