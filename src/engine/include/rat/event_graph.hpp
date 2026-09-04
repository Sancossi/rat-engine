#pragma once

#include "rat/map_data.hpp"
#include "rat/map_document.hpp"

#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] Command command_from_node(const EventGraphNode& node);
[[nodiscard]] const EventGraphNode* find_graph_node(const EventGraph& graph, std::string_view id);
[[nodiscard]] std::optional<std::string> graph_sequence_successor(const EventGraph& graph,
                                                                  std::string_view node_id);
[[nodiscard]] std::optional<std::string> graph_then_target(const EventGraph& graph,
                                                           std::string_view node_id);
[[nodiscard]] std::optional<std::string> graph_else_target(const EventGraph& graph,
                                                           std::string_view node_id);

}  // namespace rat
