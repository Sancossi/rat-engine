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

// Graph-space layout for editor node cards (imgui-free). Pin X/Y are relative to the
// node top-left; pin centers sit fully outside the card so editor child windows do not
// eat the inner half of the hit circle.
struct EventGraphNodeMetrics {
  float width = 220.0f;
  float height = 36.0f;
  float title_h = 22.0f;
  float pin_r = 7.0f;
  float in_pin_x = 0.0f;
  float out_pin_x = 0.0f;
  float in_pin_y = 18.0f;
  float seq_out_pin_y = 18.0f;
  float then_pin_y = 0.0f;
  float else_pin_y = 0.0f;
};

[[nodiscard]] EventGraphNodeMetrics event_graph_node_metrics(std::string_view kind,
                                                            std::size_t route_step_count = 0);

}  // namespace rat
