#include "event_graph_edit.hpp"

#include <rat/event_graph.hpp>
#include <rat/map_document.hpp>

#include <algorithm>
#include <utility>

namespace rat {
namespace {

[[nodiscard]] bool is_reserved_graph_id(std::string_view id) {
  return id == kEventGraphEntryId || id == kEventGraphExitId;
}

[[nodiscard]] bool graph_has_id(const EventGraph& graph, std::string_view id) {
  if (is_reserved_graph_id(id)) {
    return true;
  }
  for (const EventGraphNode& node : graph.nodes) {
    if (node.id == id) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool same_edge_branch(const std::optional<std::string>& a,
                                     const std::optional<std::string>& b) {
  const bool a_seq = !a.has_value() || a->empty();
  const bool b_seq = !b.has_value() || b->empty();
  if (a_seq && b_seq) {
    return true;
  }
  if (a_seq || b_seq) {
    return false;
  }
  return *a == *b;
}

[[nodiscard]] std::optional<std::string> normalize_branch(std::optional<std::string> branch) {
  if (!branch.has_value() || branch->empty()) {
    return std::nullopt;
  }
  return branch;
}

}  // namespace

bool is_mvp_event_graph_kind(std::string_view kind) {
  return kind == "show_text" || kind == "control_switch" || kind == "conditional_branch" ||
         kind == "wait";
}

void ensure_event_page_graph(EventPage& page) {
  if (!page.graph.has_value()) {
    page.graph = EventGraph{};
  }
}

std::string allocate_event_graph_node_id(const EventGraph& graph) {
  for (int i = 1; i < 10000; ++i) {
    std::string id = "n" + std::to_string(i);
    if (!graph_has_id(graph, id)) {
      return id;
    }
  }
  return {};
}

std::string add_event_graph_node(EventGraph& graph, std::string_view kind) {
  if (!is_mvp_event_graph_kind(kind)) {
    return {};
  }
  EventGraphNode node;
  node.id = allocate_event_graph_node_id(graph);
  if (node.id.empty()) {
    return {};
  }
  node.kind = std::string(kind);
  if (kind == "show_text") {
    node.text = "New text";
  } else if (kind == "control_switch") {
    node.switch_id = 1;
    node.bool_value = true;
  } else if (kind == "wait") {
    node.frames = 60;
  } else if (kind == "conditional_branch") {
    node.branch_condition.type = ConditionType::Switch;
    node.branch_condition.id = 1;
    node.branch_condition.bool_value = true;
  }
  graph.nodes.push_back(node);
  return node.id;
}

bool connect_event_graph_nodes(EventGraph& graph, std::string from, std::string to,
                               std::optional<std::string> branch) {
  if (from.empty() || to.empty() || from == to) {
    return false;
  }
  if (from == kEventGraphExitId || to == kEventGraphEntryId) {
    return false;
  }
  if (!graph_has_id(graph, from) || !graph_has_id(graph, to)) {
    return false;
  }
  std::optional<std::string> role = normalize_branch(std::move(branch));
  if (role.has_value() && *role != "then" && *role != "else") {
    return false;
  }
  for (EventGraphEdge& edge : graph.edges) {
    if (edge.from == from && same_edge_branch(edge.branch, role)) {
      edge.to = std::move(to);
      return true;
    }
  }
  EventGraphEdge edge;
  edge.from = std::move(from);
  edge.to = std::move(to);
  edge.branch = std::move(role);
  graph.edges.push_back(std::move(edge));
  return true;
}

bool delete_event_graph_node(EventGraph& graph, std::string_view id) {
  if (id.empty() || is_reserved_graph_id(id)) {
    return false;
  }
  const auto node = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                  [&](const EventGraphNode& n) { return n.id == id; });
  if (node == graph.nodes.end()) {
    return false;
  }
  graph.nodes.erase(node);
  graph.edges.erase(std::remove_if(graph.edges.begin(), graph.edges.end(),
                                   [&](const EventGraphEdge& edge) {
                                     return edge.from == id || edge.to == id;
                                   }),
                     graph.edges.end());
  return true;
}

bool delete_event_graph_edge(EventGraph& graph, std::string_view from, std::string_view to,
                             std::optional<std::string> branch) {
  const std::optional<std::string> role = normalize_branch(std::move(branch));
  const auto edge = std::find_if(graph.edges.begin(), graph.edges.end(),
                                   [&](const EventGraphEdge& candidate) {
                                     return candidate.from == from && candidate.to == to &&
                                            same_edge_branch(candidate.branch, role);
                                   });
  if (edge == graph.edges.end()) {
    return false;
  }
  graph.edges.erase(edge);
  return true;
}

EventGraphApplyResult compile_event_graphs_for_apply(const MapData& map) {
  EventGraphApplyResult result;
  result.map = map;
  std::vector<MapIssue> issues;
  bool failed = false;
  for (std::size_t e = 0; e < result.map.events.size(); ++e) {
    EventDef& event = result.map.events[e];
    for (std::size_t p = 0; p < event.pages.size(); ++p) {
      EventPage& page = event.pages[p];
      if (!page.graph.has_value()) {
        continue;
      }
      const EventGraphCompileResult compiled = compile_event_graph(*page.graph);
      const std::string prefix =
          "/events/" + std::to_string(e) + "/pages/" + std::to_string(p) + "/graph";
      for (MapIssue issue : compiled.issues) {
        issue.json_path = prefix + issue.json_path;
        issues.push_back(std::move(issue));
      }
      if (!compiled.ok) {
        failed = true;
        continue;
      }
      page.commands = compiled.commands;
    }
  }
  result.issues = std::move(issues);
  if (failed) {
    result.ok = false;
    result.map = map;
    return result;
  }
  result.ok = true;
  return result;
}

}  // namespace rat
