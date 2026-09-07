#pragma once

#include <rat/event_graph.hpp>
#include <rat/map_data.hpp>
#include <rat/map_document.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

inline constexpr const char* kEventGraphEntryId = "entry";
inline constexpr const char* kEventGraphExitId = "exit";

struct EventGraphWireState {
  std::string pending_from;
  std::optional<std::string> pending_branch;
  bool dragging_wire = false;
  std::string drag_wire_from;
  std::optional<std::string> drag_wire_branch;
  bool wire_moved = false;
};
// A plain click on the source arms two-click connection; every completed drag cancels it.
void finish_event_graph_wire_release(EventGraphWireState& state, bool source_click);

struct EventGraphApplyResult {
  bool ok = false;
  MapData map;
  std::vector<MapIssue> issues;
};

[[nodiscard]] bool is_mvp_event_graph_kind(std::string_view kind);

void ensure_event_page_graph(EventPage& page);

[[nodiscard]] std::string allocate_event_graph_node_id(const EventGraph& graph);

[[nodiscard]] std::string add_event_graph_node(EventGraph& graph, std::string_view kind);

bool connect_event_graph_nodes(EventGraph& graph, std::string from, std::string to,
                               std::optional<std::string> branch = {});

[[nodiscard]] std::string duplicate_event_graph_node(EventGraph& graph, std::string_view id);

[[nodiscard]] bool event_graph_pin_contains(float center_x, float center_y, float radius,
                                            float mouse_x, float mouse_y);

enum class EventGraphCanvasHistoryAction { None, Undo, Redo };

[[nodiscard]] EventGraphCanvasHistoryAction event_graph_canvas_history_action(bool ctrl, bool shift,
                                                                              bool z, bool y);

bool delete_event_graph_node(EventGraph& graph, std::string_view id);

bool delete_event_graph_edge(EventGraph& graph, std::string_view from, std::string_view to,
                             std::optional<std::string> branch = {});

[[nodiscard]] EventGraphApplyResult compile_event_graphs_for_apply(const MapData& map);

}  // namespace rat
