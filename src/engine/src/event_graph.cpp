#include "rat/event_graph.hpp"

#include <algorithm>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rat {
namespace {

constexpr const char* kEntry = "entry";
constexpr const char* kExit = "exit";

enum class EdgeRole {
  Sequence,
  Then,
  Else,
};

struct IndexedEdge {
  std::string to;
  std::optional<int> order;
  EdgeRole role = EdgeRole::Sequence;
  std::size_t source_index = 0;
};

[[nodiscard]] bool is_pseudo_id(std::string_view id) {
  return id == kEntry || id == kExit;
}

[[nodiscard]] bool is_allowed_kind(std::string_view kind) {
  return kind == "show_text" || kind == "control_switch" || kind == "control_variable" ||
         kind == "control_self_switch" || kind == "conditional_branch" || kind == "wait" ||
         kind == "transfer_player" || kind == "change_items" || kind == "play_se" ||
         kind == "set_move_route" || kind == "comment";
}

[[nodiscard]] std::string index_path(std::string_view prefix, std::size_t index) {
  std::string path;
  path.reserve(prefix.size() + 8);
  path.append(prefix);
  path.push_back('/');
  path.append(std::to_string(index));
  return path;
}

void add_error(EventGraphCompileResult& result, std::string json_path, std::string message) {
  MapIssue issue;
  issue.severity = MapIssueSeverity::Error;
  issue.json_path = std::move(json_path);
  issue.message = std::move(message);
  result.issues.push_back(std::move(issue));
}

[[nodiscard]] EdgeRole parse_edge_role(const std::optional<std::string>& branch, bool& ok) {
  if (!branch.has_value() || branch->empty()) {
    return EdgeRole::Sequence;
  }
  if (*branch == "then") {
    return EdgeRole::Then;
  }
  if (*branch == "else") {
    return EdgeRole::Else;
  }
  ok = false;
  return EdgeRole::Sequence;
}

[[nodiscard]] bool edge_order_less(const IndexedEdge& a, const IndexedEdge& b) {
  const int oa = a.order.value_or(0);
  const int ob = b.order.value_or(0);
  if (oa != ob) {
    return oa < ob;
  }
  if (a.to != b.to) {
    return a.to < b.to;
  }
  return a.source_index < b.source_index;
}

[[nodiscard]] bool is_sequence_edge(const EventGraphEdge& edge) {
  return !edge.branch.has_value() || edge.branch->empty();
}

[[nodiscard]] bool is_then_edge(const EventGraphEdge& edge) {
  return edge.branch.has_value() && *edge.branch == "then";
}

[[nodiscard]] bool is_else_edge(const EventGraphEdge& edge) {
  return edge.branch.has_value() && *edge.branch == "else";
}

[[nodiscard]] bool edge_better(const EventGraphEdge& candidate, std::size_t candidate_index,
                               const EventGraphEdge& best, std::size_t best_index) {
  const int oc = candidate.order.value_or(0);
  const int ob = best.order.value_or(0);
  if (oc != ob) {
    return oc < ob;
  }
  if (candidate.to != best.to) {
    return candidate.to < best.to;
  }
  return candidate_index < best_index;
}

[[nodiscard]] std::optional<std::string> first_edge_target(
    const EventGraph& graph, std::string_view from,
    bool (*matches)(const EventGraphEdge&)) {
  const EventGraphEdge* best = nullptr;
  std::size_t best_index = 0;
  for (std::size_t i = 0; i < graph.edges.size(); ++i) {
    const EventGraphEdge& edge = graph.edges[i];
    if (edge.from != from || !matches(edge)) {
      continue;
    }
    if (best == nullptr || edge_better(edge, i, *best, best_index)) {
      best = &edge;
      best_index = i;
    }
  }
  if (best == nullptr) {
    return std::nullopt;
  }
  return best->to;
}

[[nodiscard]] EventGraphNode node_from_command(const Command& command) {
  EventGraphNode node;
  if (command.op == CommandOp::ShowText) {
    node.kind = "show_text";
    node.text = command.text;
  } else if (command.op == CommandOp::ControlSwitch) {
    node.kind = "control_switch";
    node.switch_id = command.id;
    node.bool_value = command.bool_value;
  } else if (command.op == CommandOp::ControlVariable) {
    node.kind = "control_variable";
    node.switch_id = command.id;
    node.int_value = command.int_value;
  } else if (command.op == CommandOp::ControlSelfSwitch) {
    node.kind = "control_self_switch";
    node.self_switch = command.self_switch;
    node.bool_value = command.bool_value;
  } else if (command.op == CommandOp::Wait) {
    node.kind = "wait";
    node.frames = command.frames;
  } else if (command.op == CommandOp::ConditionalBranch) {
    node.kind = "conditional_branch";
    node.branch_condition = command.branch_condition;
  } else if (command.op == CommandOp::TransferPlayer) {
    node.kind = "transfer_player";
    node.map_id = command.map_id;
    node.x = command.x;
    node.y = command.y;
    node.z = command.z;
  } else if (command.op == CommandOp::ChangeItems) {
    node.kind = "change_items";
    node.item_id = command.item_id;
    node.item_delta = command.item_delta;
    node.key_item = command.key_item;
  } else if (command.op == CommandOp::PlaySE) {
    node.kind = "play_se";
    node.text = command.text;
  } else if (command.op == CommandOp::SetMoveRoute) {
    node.kind = "set_move_route";
    node.through = command.through;
    node.route = command.route;
  } else {
    node.kind = "comment";
    node.text = command.text;
  }
  return node;
}

struct ReverseCompiler {
  EventGraph graph;
  int next_id = 1;

  [[nodiscard]] std::string alloc_id() {
    std::string id = "n";
    id += std::to_string(next_id++);
    return id;
  }

  [[nodiscard]] std::string emit_list(const std::vector<Command>& commands,
                                      const std::string& join) {
    if (commands.empty()) {
      return join;
    }

    std::vector<std::string> ids;
    ids.reserve(commands.size());
    for (const Command& command : commands) {
      EventGraphNode node = node_from_command(command);
      node.id = alloc_id();
      ids.push_back(node.id);
      graph.nodes.push_back(std::move(node));
    }

    for (std::size_t i = 0; i < commands.size(); ++i) {
      const std::string& id = ids[i];
      const Command& command = commands[i];
      const std::string next = (i + 1 < commands.size()) ? ids[i + 1] : join;

      if (command.op == CommandOp::ConditionalBranch) {
        const std::string then_head = emit_list(command.then_commands, next);
        EventGraphEdge then_edge;
        then_edge.from = id;
        then_edge.to = then_head;
        then_edge.branch = std::string("then");
        graph.edges.push_back(std::move(then_edge));

        if (!command.else_commands.empty()) {
          const std::string else_head = emit_list(command.else_commands, next);
          EventGraphEdge else_edge;
          else_edge.from = id;
          else_edge.to = else_head;
          else_edge.branch = std::string("else");
          graph.edges.push_back(std::move(else_edge));
        } else if (next != kExit) {
          EventGraphEdge else_edge;
          else_edge.from = id;
          else_edge.to = next;
          else_edge.branch = std::string("else");
          graph.edges.push_back(std::move(else_edge));
        }
      } else {
        EventGraphEdge seq;
        seq.from = id;
        seq.to = next;
        graph.edges.push_back(std::move(seq));
      }
    }

    return ids.front();
  }
};

struct GraphIndex {
  std::unordered_map<std::string, std::size_t> node_index;
  std::unordered_map<std::string, std::vector<IndexedEdge>> outgoing;
};

[[nodiscard]] std::vector<IndexedEdge> edges_with_role(const GraphIndex& index,
                                                       const std::string& from, EdgeRole role) {
  std::vector<IndexedEdge> selected;
  const auto found = index.outgoing.find(from);
  if (found == index.outgoing.end()) {
    return selected;
  }
  for (const IndexedEdge& edge : found->second) {
    if (edge.role == role) {
      selected.push_back(edge);
    }
  }
  std::sort(selected.begin(), selected.end(), edge_order_less);
  return selected;
}

[[nodiscard]] std::vector<std::string> successor_ids(const GraphIndex& index,
                                                     const std::string& from) {
  std::vector<std::string> ids;
  const auto found = index.outgoing.find(from);
  if (found == index.outgoing.end()) {
    return ids;
  }
  ids.reserve(found->second.size());
  for (const IndexedEdge& edge : found->second) {
    ids.push_back(edge.to);
  }
  return ids;
}

[[nodiscard]] bool reaches_exit(const GraphIndex& index, const std::string& start,
                                const std::string& blocked) {
  if (start == kExit && start != blocked) {
    return true;
  }
  std::unordered_set<std::string> visited;
  std::vector<std::string> stack;
  stack.push_back(start);
  while (!stack.empty()) {
    const std::string id = std::move(stack.back());
    stack.pop_back();
    if (id == blocked || !visited.insert(id).second) {
      continue;
    }
    if (id == kExit) {
      return true;
    }
    for (const std::string& next : successor_ids(index, id)) {
      stack.push_back(next);
    }
  }
  return false;
}

[[nodiscard]] std::unordered_map<std::string, int> distances_from(const GraphIndex& index,
                                                                  const std::string& start) {
  std::unordered_map<std::string, int> distance;
  std::queue<std::string> pending;
  distance[start] = 0;
  pending.push(start);
  while (!pending.empty()) {
    const std::string id = pending.front();
    pending.pop();
    const int next_dist = distance[id] + 1;
    for (const std::string& next : successor_ids(index, id)) {
      if (distance.find(next) != distance.end()) {
        continue;
      }
      distance[next] = next_dist;
      pending.push(next);
    }
  }
  return distance;
}

[[nodiscard]] std::string join_after_branch(const GraphIndex& index, const EventGraph& graph,
                                            const std::string& branch_id) {
  if (edges_with_role(index, branch_id, EdgeRole::Else).empty()) {
    return kExit;
  }

  std::vector<std::string> candidates;
  candidates.reserve(graph.nodes.size() + 1);
  for (const EventGraphNode& node : graph.nodes) {
    if (node.id != branch_id) {
      candidates.push_back(node.id);
    }
  }
  candidates.emplace_back(kExit);

  std::vector<std::string> postdominators;
  for (const std::string& candidate : candidates) {
    if (!reaches_exit(index, branch_id, candidate)) {
      postdominators.push_back(candidate);
    }
  }
  if (postdominators.empty()) {
    return kExit;
  }

  const std::unordered_map<std::string, int> distance = distances_from(index, branch_id);
  std::string best = postdominators.front();
  for (const std::string& candidate : postdominators) {
    const int best_d = distance.find(best) == distance.end() ? 1'000'000 : distance.at(best);
    const int cand_d =
        distance.find(candidate) == distance.end() ? 1'000'000 : distance.at(candidate);
    if (cand_d < best_d || (cand_d == best_d && candidate < best)) {
      best = candidate;
    }
  }
  return best;
}

struct Emitter {
  const EventGraph& graph;
  const GraphIndex& index;
  EventGraphCompileResult& result;

  [[nodiscard]] const EventGraphNode* find_node(const std::string& id) const {
    const auto found = index.node_index.find(id);
    if (found == index.node_index.end()) {
      return nullptr;
    }
    return &graph.nodes[found->second];
  }

  std::vector<Command> emit_chain(const std::string& id, const std::unordered_set<std::string>& stop,
                                  std::unordered_set<std::string>& path) {
    if (id.empty() || id == kExit || stop.find(id) != stop.end()) {
      return {};
    }
    if (is_pseudo_id(id) && id == kEntry) {
      std::vector<Command> commands;
      for (const IndexedEdge& edge : edges_with_role(index, kEntry, EdgeRole::Sequence)) {
        const std::vector<Command> more = emit_chain(edge.to, stop, path);
        commands.insert(commands.end(), more.begin(), more.end());
      }
      return commands;
    }

    const EventGraphNode* node = find_node(id);
    if (node == nullptr) {
      return {};
    }
    if (!path.insert(id).second) {
      return {};
    }

    std::vector<Command> commands;
    if (node->kind == "conditional_branch") {
      Command command = command_from_node(*node);
      const std::string join = join_after_branch(index, graph, id);
      std::unordered_set<std::string> inner_stop = stop;
      inner_stop.insert(join);
      for (const IndexedEdge& edge : edges_with_role(index, id, EdgeRole::Then)) {
        const std::vector<Command> then_cmds = emit_chain(edge.to, inner_stop, path);
        command.then_commands.insert(command.then_commands.end(), then_cmds.begin(),
                                     then_cmds.end());
      }
      for (const IndexedEdge& edge : edges_with_role(index, id, EdgeRole::Else)) {
        const std::vector<Command> else_cmds = emit_chain(edge.to, inner_stop, path);
        command.else_commands.insert(command.else_commands.end(), else_cmds.begin(),
                                     else_cmds.end());
      }
      commands.push_back(std::move(command));
      if (join != kExit && stop.find(join) == stop.end()) {
        const std::vector<Command> after = emit_chain(join, stop, path);
        commands.insert(commands.end(), after.begin(), after.end());
      }
    } else {
      commands.push_back(command_from_node(*node));
      for (const IndexedEdge& edge : edges_with_role(index, id, EdgeRole::Sequence)) {
        const std::vector<Command> more = emit_chain(edge.to, stop, path);
        commands.insert(commands.end(), more.begin(), more.end());
      }
    }

    path.erase(id);
    return commands;
  }
};

void validate_and_index(const EventGraph& graph, GraphIndex& index,
                        EventGraphCompileResult& result) {
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const EventGraphNode& node = graph.nodes[i];
    const std::string node_path = index_path("/nodes", i);
    if (node.id.empty()) {
      add_error(result, node_path + "/id", "graph node id must not be empty");
      continue;
    }
    if (is_pseudo_id(node.id)) {
      add_error(result, node_path + "/id", "graph node id 'entry' and 'exit' are reserved");
      continue;
    }
    if (!index.node_index.emplace(node.id, i).second) {
      add_error(result, node_path + "/id", "graph node id must be unique");
      continue;
    }
    if (!is_allowed_kind(node.kind)) {
      add_error(result, node_path + "/kind", "unknown graph node kind: " + node.kind);
    }
    if (node.kind == "wait" && node.frames < 0) {
      add_error(result, node_path + "/params/frames", "wait frames must be >= 0");
    }
  }

  for (std::size_t i = 0; i < graph.edges.size(); ++i) {
    const EventGraphEdge& edge = graph.edges[i];
    const std::string edge_path = index_path("/edges", i);
    if (edge.from.empty()) {
      add_error(result, edge_path + "/from", "graph edge from must not be empty");
      continue;
    }
    if (edge.to.empty()) {
      add_error(result, edge_path + "/to", "graph edge to must not be empty");
      continue;
    }
    bool role_ok = true;
    const EdgeRole role = parse_edge_role(edge.branch, role_ok);
    if (!role_ok) {
      add_error(result, edge_path + "/branch", "graph edge branch must be then or else");
      continue;
    }
    if (!is_pseudo_id(edge.from) && index.node_index.find(edge.from) == index.node_index.end()) {
      add_error(result, edge_path + "/from", "graph edge from unknown node: " + edge.from);
      continue;
    }
    if (!is_pseudo_id(edge.to) && index.node_index.find(edge.to) == index.node_index.end()) {
      add_error(result, edge_path + "/to", "graph edge to unknown node: " + edge.to);
      continue;
    }
    const auto from_node = index.node_index.find(edge.from);
    if (role != EdgeRole::Sequence) {
      if (from_node == index.node_index.end() ||
          graph.nodes[from_node->second].kind != "conditional_branch") {
        add_error(result, edge_path + "/branch",
                  "then/else edges must start at a conditional_branch node");
        continue;
      }
    } else if (from_node != index.node_index.end() &&
               graph.nodes[from_node->second].kind == "conditional_branch") {
      add_error(result, edge_path + "/branch",
                "sequence edge cannot leave a conditional_branch node");
      continue;
    }
    IndexedEdge indexed;
    indexed.to = edge.to;
    indexed.order = edge.order;
    indexed.role = role;
    indexed.source_index = i;
    index.outgoing[edge.from].push_back(std::move(indexed));
  }

  for (auto& [from, edges] : index.outgoing) {
    std::stable_sort(edges.begin(), edges.end(), edge_order_less);
    (void)from;
  }

  enum class Color { White, Gray, Black };
  std::unordered_map<std::string, Color> color;
  color[kEntry] = Color::White;
  color[kExit] = Color::White;
  for (const EventGraphNode& node : graph.nodes) {
    if (!node.id.empty()) {
      color[node.id] = Color::White;
    }
  }

  std::vector<std::string> path;
  std::unordered_set<std::string> reachable;

  const auto record_cycle = [&](const std::string& back_to) {
    std::vector<std::string> cycle_ids;
    bool recording = false;
    for (const std::string& id : path) {
      if (id == back_to) {
        recording = true;
      }
      if (recording) {
        cycle_ids.push_back(id);
      }
    }
    cycle_ids.push_back(back_to);
    bool has_wait = false;
    for (const std::string& id : cycle_ids) {
      const auto found = index.node_index.find(id);
      if (found != index.node_index.end() && graph.nodes[found->second].kind == "wait") {
        has_wait = true;
        break;
      }
    }
    if (has_wait) {
      add_error(result, "/edges", "graph cycle cannot be compiled into a command list");
    } else {
      add_error(result, "/edges", "cycle without Wait");
    }
  };

  const auto dfs = [&](auto&& self, const std::string& id) -> void {
    color[id] = Color::Gray;
    path.push_back(id);
    reachable.insert(id);
    for (const std::string& next : successor_ids(index, id)) {
      const Color next_color = color.find(next) == color.end() ? Color::White : color[next];
      if (next_color == Color::Gray) {
        record_cycle(next);
        continue;
      }
      if (next_color == Color::White) {
        self(self, next);
      } else {
        reachable.insert(next);
      }
    }
    path.pop_back();
    color[id] = Color::Black;
  };
  dfs(dfs, kEntry);

  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const EventGraphNode& node = graph.nodes[i];
    if (node.id.empty()) {
      continue;
    }
    if (reachable.find(node.id) == reachable.end()) {
      add_error(result, index_path("/nodes", i), "unreachable graph node: " + node.id);
    }
  }

  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const EventGraphNode& node = graph.nodes[i];
    if (node.kind != "conditional_branch") {
      continue;
    }
    if (edges_with_role(index, node.id, EdgeRole::Then).empty()) {
      add_error(result, index_path("/nodes", i), "conditional_branch requires a then-edge");
    }
  }
}

}  // namespace

EventGraphCompileResult compile_event_graph(const EventGraph& graph) {
  EventGraphCompileResult result;
  GraphIndex index;
  validate_and_index(graph, index, result);
  if (!result.issues.empty()) {
    result.ok = false;
    return result;
  }

  Emitter emitter{graph, index, result};
  std::unordered_set<std::string> path;
  result.commands = emitter.emit_chain(kEntry, {}, path);
  result.ok = result.issues.empty();
  return result;
}

EventGraph commands_to_graph(const std::vector<Command>& commands) {
  ReverseCompiler compiler;
  const std::string head = compiler.emit_list(commands, kExit);
  EventGraphEdge entry;
  entry.from = kEntry;
  entry.to = head;
  compiler.graph.edges.insert(compiler.graph.edges.begin(), std::move(entry));
  return compiler.graph;
}

void ensure_page_graph_from_commands(EventPage& page) {
  if (page.graph.has_value() && !page.graph->nodes.empty()) {
    return;
  }
  page.graph = commands_to_graph(page.commands);
}

Command command_from_node(const EventGraphNode& node) {
  Command command;
  if (node.kind == "show_text") {
    command.op = CommandOp::ShowText;
    command.text = node.text;
  } else if (node.kind == "control_switch") {
    command.op = CommandOp::ControlSwitch;
    command.id = node.switch_id;
    command.bool_value = node.bool_value;
  } else if (node.kind == "control_variable") {
    command.op = CommandOp::ControlVariable;
    command.id = node.switch_id;
    command.int_value = node.int_value;
  } else if (node.kind == "control_self_switch") {
    command.op = CommandOp::ControlSelfSwitch;
    command.self_switch = node.self_switch;
    command.bool_value = node.bool_value;
  } else if (node.kind == "wait") {
    command.op = CommandOp::Wait;
    command.frames = node.frames;
  } else if (node.kind == "conditional_branch") {
    command.op = CommandOp::ConditionalBranch;
    command.branch_condition = node.branch_condition;
  } else if (node.kind == "transfer_player") {
    command.op = CommandOp::TransferPlayer;
    command.map_id = node.map_id;
    command.x = node.x;
    command.y = node.y;
    command.z = node.z;
  } else if (node.kind == "change_items") {
    command.op = CommandOp::ChangeItems;
    command.item_id = node.item_id;
    command.item_delta = node.item_delta;
    command.key_item = node.key_item;
  } else if (node.kind == "play_se") {
    command.op = CommandOp::PlaySE;
    command.text = node.text;
  } else if (node.kind == "set_move_route") {
    command.op = CommandOp::SetMoveRoute;
    command.through = node.through;
    command.route = node.route;
  } else if (node.kind == "comment") {
    command.op = CommandOp::Comment;
    command.text = node.text;
  } else {
    command.op = CommandOp::Comment;
    command.text = node.kind;
  }
  return command;
}

const EventGraphNode* find_graph_node(const EventGraph& graph, std::string_view id) {
  for (const EventGraphNode& node : graph.nodes) {
    if (node.id == id) {
      return &node;
    }
  }
  return nullptr;
}

std::optional<std::string> graph_sequence_successor(const EventGraph& graph,
                                                    std::string_view node_id) {
  return first_edge_target(graph, node_id, is_sequence_edge);
}

std::optional<std::string> graph_then_target(const EventGraph& graph, std::string_view node_id) {
  return first_edge_target(graph, node_id, is_then_edge);
}

std::optional<std::string> graph_else_target(const EventGraph& graph, std::string_view node_id) {
  return first_edge_target(graph, node_id, is_else_edge);
}

EventGraphNodeMetrics event_graph_node_metrics(std::string_view kind,
                                               std::size_t route_step_count) {
  constexpr float kWidth = 220.0f;
  constexpr float kTitleH = 22.0f;
  constexpr float kPad = 8.0f;
  constexpr float kRowH = 24.0f;
  constexpr float kGap = 4.0f;
  constexpr float kMultilineH = 52.0f;

  struct Row {
    float height = kRowH;
    bool then_pin = false;
    bool else_pin = false;
  };
  std::vector<Row> rows;
  if (kind == "show_text" || kind == "comment") {
    rows.push_back(Row{kMultilineH, false, false});
  } else if (kind == "wait" || kind == "play_se") {
    rows.push_back(Row{});
  } else if (kind == "control_switch" || kind == "control_variable" ||
             kind == "control_self_switch") {
    rows.push_back(Row{});
    rows.push_back(Row{});
  } else if (kind == "conditional_branch") {
    rows.push_back(Row{});
    rows.push_back(Row{});
    rows.push_back(Row{kRowH, true, false});
    rows.push_back(Row{kRowH, false, true});
  } else if (kind == "transfer_player") {
    rows.push_back(Row{});
    rows.push_back(Row{});
    rows.push_back(Row{});
    rows.push_back(Row{});
  } else if (kind == "change_items") {
    rows.push_back(Row{});
    rows.push_back(Row{});
    rows.push_back(Row{});
  } else if (kind == "set_move_route") {
    rows.push_back(Row{});
    for (std::size_t i = 0; i < route_step_count; ++i) {
      rows.push_back(Row{});
    }
    rows.push_back(Row{});
  }

  EventGraphNodeMetrics metrics;
  metrics.width = kWidth;
  metrics.title_h = kTitleH;
  float y = kTitleH + kPad;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const float mid = y + rows[i].height * 0.5f;
    if (rows[i].then_pin) {
      metrics.then_pin_y = mid;
    }
    if (rows[i].else_pin) {
      metrics.else_pin_y = mid;
    }
    y += rows[i].height;
    if (i + 1 < rows.size()) {
      y += kGap;
    }
  }
  y += kPad;
  if (rows.empty()) {
    y = kTitleH + kPad;
  }
  metrics.height = y;
  metrics.in_pin_x = -metrics.pin_r;
  metrics.out_pin_x = metrics.width + metrics.pin_r;
  metrics.in_pin_y = metrics.height * 0.5f;
  metrics.seq_out_pin_y = metrics.height * 0.5f;
  return metrics;
}

}  // namespace rat
