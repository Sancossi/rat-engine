#include "rat/map_loader.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace rat {
namespace {

using json = nlohmann::json;

MapLoadResult fail(std::string message) {
  MapLoadResult result;
  result.ok = false;
  result.error = std::move(message);
  return result;
}

CompareOp parse_compare_op(const std::string& op) {
  if (op == "==") {
    return CompareOp::Eq;
  }
  if (op == "!=") {
    return CompareOp::Ne;
  }
  if (op == "<") {
    return CompareOp::Lt;
  }
  if (op == "<=") {
    return CompareOp::Le;
  }
  if (op == ">") {
    return CompareOp::Gt;
  }
  if (op == ">=") {
    return CompareOp::Ge;
  }
  throw std::runtime_error("unknown variable op: " + op);
}

TriggerKind parse_trigger(const std::string& value) {
  if (value == "action") {
    return TriggerKind::Action;
  }
  if (value == "player_touch") {
    return TriggerKind::PlayerTouch;
  }
  if (value == "event_touch") {
    return TriggerKind::EventTouch;
  }
  if (value == "autorun") {
    return TriggerKind::Autorun;
  }
  if (value == "parallel") {
    return TriggerKind::Parallel;
  }
  throw std::runtime_error("unknown trigger: " + value);
}

RampDirection parse_ramp_direction(const std::string& value) {
  if (value == "north") {
    return RampDirection::North;
  }
  if (value == "east") {
    return RampDirection::East;
  }
  if (value == "south") {
    return RampDirection::South;
  }
  if (value == "west") {
    return RampDirection::West;
  }
  throw std::runtime_error("unknown ramp direction: " + value);
}

Condition parse_condition(const json& node) {
  Condition condition;
  const std::string type = node.at("type").get<std::string>();
  if (type == "switch") {
    condition.type = ConditionType::Switch;
    condition.id = node.at("id").get<std::uint32_t>();
    condition.bool_value = node.at("value").get<bool>();
  } else if (type == "variable") {
    condition.type = ConditionType::Variable;
    condition.id = node.at("id").get<std::uint32_t>();
    condition.op = parse_compare_op(node.at("op").get<std::string>());
    condition.int_value = node.at("value").get<int>();
  } else if (type == "item") {
    condition.type = ConditionType::Item;
    condition.string_id = node.at("id").get<std::string>();
    condition.int_value = node.value("quantity", 1);
  } else if (type == "self_switch") {
    condition.type = ConditionType::SelfSwitch;
    const std::string key = node.at("key").get<std::string>();
    if (key.size() != 1 || key[0] < 'A' || key[0] > 'D') {
      throw std::runtime_error("self_switch key must be A-D");
    }
    condition.self_switch = key[0];
    condition.bool_value = node.at("value").get<bool>();
  } else {
    throw std::runtime_error("unknown condition type: " + type);
  }
  return condition;
}

Command parse_command(const json& node);

std::vector<Command> parse_commands(const json& node) {
  std::vector<Command> commands;
  if (!node.is_array()) {
    throw std::runtime_error("commands must be an array");
  }
  commands.reserve(node.size());
  for (const auto& item : node) {
    commands.push_back(parse_command(item));
  }
  return commands;
}

Command parse_command(const json& node) {
  Command command;
  const std::string op = node.at("op").get<std::string>();
  if (op == "show_text") {
    command.op = CommandOp::ShowText;
    command.text = node.at("text").get<std::string>();
  } else if (op == "control_switch") {
    command.op = CommandOp::ControlSwitch;
    command.id = node.at("id").get<std::uint32_t>();
    command.bool_value = node.at("value").get<bool>();
  } else if (op == "control_variable") {
    command.op = CommandOp::ControlVariable;
    command.id = node.at("id").get<std::uint32_t>();
    command.int_value = node.at("value").get<int>();
  } else if (op == "control_self_switch") {
    command.op = CommandOp::ControlSelfSwitch;
    const std::string key = node.at("key").get<std::string>();
    if (key.size() != 1 || key[0] < 'A' || key[0] > 'D') {
      throw std::runtime_error("control_self_switch key must be A-D");
    }
    command.self_switch = key[0];
    command.bool_value = node.at("value").get<bool>();
  } else if (op == "conditional_branch") {
    command.op = CommandOp::ConditionalBranch;
    command.branch_condition = parse_condition(node.at("condition"));
    command.then_commands = parse_commands(node.at("then"));
    if (node.contains("else")) {
      command.else_commands = parse_commands(node.at("else"));
    }
  } else if (op == "wait") {
    command.op = CommandOp::Wait;
    command.frames = node.at("frames").get<int>();
    if (command.frames < 0) {
      throw std::runtime_error("wait frames must be >= 0");
    }
  } else if (op == "transfer_player") {
    command.op = CommandOp::TransferPlayer;
    command.map_id = node.at("map_id").get<std::string>();
    command.x = node.at("x").get<float>();
    command.y = node.value("y", 0.0f);
    command.z = node.at("z").get<float>();
  } else if (op == "change_items") {
    command.op = CommandOp::ChangeItems;
    command.item_id = node.at("id").get<std::string>();
    command.item_delta = node.at("delta").get<int>();
    command.key_item = node.value("key_item", false);
  } else if (op == "play_se") {
    command.op = CommandOp::PlaySE;
    command.text = node.at("id").get<std::string>();
    if (command.text.empty()) {
      throw std::runtime_error("play_se id must not be empty");
    }
  } else if (op == "comment") {
    command.op = CommandOp::Comment;
    command.text = node.value("text", "");
  } else {
    throw std::runtime_error("unknown command op: " + op);
  }
  return command;
}

Aabb2 parse_aabb(const json& node) {
  return Aabb2{
      node.at("min_x").get<float>(),
      node.at("min_z").get<float>(),
      node.at("max_x").get<float>(),
      node.at("max_z").get<float>(),
  };
}

BlockerDef parse_blocker(const json& node) {
  BlockerDef blocker;
  blocker.bounds = parse_aabb(node);

  const bool has_base_y = node.contains("base_y");
  const bool has_top_y = node.contains("top_y");
  const bool has_jumpable = node.contains("jumpable");

  if (has_base_y) {
    blocker.base_y = node.at("base_y").get<float>();
  }
  if (has_top_y) {
    blocker.top_y = node.at("top_y").get<float>();
  }
  if (has_jumpable) {
    blocker.jumpable = node.at("jumpable").get<bool>();
  }

  if (has_base_y != has_top_y) {
    throw std::runtime_error("blocker vertical fields require base_y and top_y pair");
  }
  if (blocker.jumpable && !has_base_y) {
    throw std::runtime_error("jumpable blocker requires base_y and top_y");
  }
  if (has_base_y && has_top_y && *blocker.top_y < *blocker.base_y) {
    throw std::runtime_error("blocker top_y must be >= base_y");
  }

  return blocker;
}

bool edge_tile_in_grid(const HeightGrid& grid, int tile_x, int tile_z) {
  const int local_x = tile_x - grid.origin_x;
  const int local_z = tile_z - grid.origin_z;
  return local_x >= 0 && local_z >= 0 && local_x < grid.width && local_z < grid.height;
}

bool edge_tile_has_ramp(const std::vector<RampDef>& ramps, int tile_x, int tile_z) {
  for (const RampDef& ramp : ramps) {
    if (ramp.tile.x == tile_x && ramp.tile.z == tile_z) {
      return true;
    }
  }
  return false;
}

void canonicalize_edge_barriers(MapData& map) {
  std::vector<EdgeBarrierDef> unique;
  unique.reserve(map.edge_barriers.size());
  for (const EdgeBarrierDef& incoming : map.edge_barriers) {
    bool replaced = false;
    for (EdgeBarrierDef& existing : unique) {
      if (existing.tile.x == incoming.tile.x && existing.tile.z == incoming.tile.z &&
          existing.direction == incoming.direction) {
        existing = incoming;
        replaced = true;
        break;
      }
    }
    if (!replaced) {
      unique.push_back(incoming);
    }
  }

  std::vector<EdgeBarrierDef> kept;
  kept.reserve(unique.size());
  for (const EdgeBarrierDef& edge : unique) {
    if (edge.height <= 0.0f) {
      continue;
    }
    if (!edge_tile_in_grid(map.height_grid, edge.tile.x, edge.tile.z)) {
      continue;
    }
    if (edge_tile_has_ramp(map.ramps, edge.tile.x, edge.tile.z)) {
      continue;
    }
    kept.push_back(edge);
  }
  map.edge_barriers = std::move(kept);
}

EventGraphNode parse_graph_node(const json& node) {
  EventGraphNode graph_node;
  graph_node.id = node.at("id").get<std::string>();
  graph_node.kind = node.at("kind").get<std::string>();
  if (!node.contains("params") || !node.at("params").is_object()) {
    return graph_node;
  }
  const json& params = node.at("params");
  if (params.contains("text")) {
    graph_node.text = params.at("text").get<std::string>();
  }
  if (params.contains("id") && params.at("id").is_number_unsigned()) {
    graph_node.switch_id = params.at("id").get<std::uint32_t>();
  }
  if (params.contains("value") && params.at("value").is_boolean()) {
    graph_node.bool_value = params.at("value").get<bool>();
  }
  if (params.contains("frames")) {
    graph_node.frames = params.at("frames").get<int>();
  }
  if (params.contains("condition")) {
    graph_node.branch_condition = parse_condition(params.at("condition"));
  }
  return graph_node;
}

EventGraphEdge parse_graph_edge(const json& node) {
  EventGraphEdge edge;
  edge.from = node.at("from").get<std::string>();
  edge.to = node.at("to").get<std::string>();
  if (node.contains("order")) {
    edge.order = node.at("order").get<int>();
  }
  if (node.contains("branch")) {
    edge.branch = node.at("branch").get<std::string>();
  }
  return edge;
}

EventGraph parse_graph(const json& node) {
  if (!node.is_object()) {
    throw std::runtime_error("graph must be an object");
  }
  EventGraph graph;
  if (node.contains("nodes")) {
    if (!node.at("nodes").is_array()) {
      throw std::runtime_error("graph nodes must be an array");
    }
    for (const auto& item : node.at("nodes")) {
      graph.nodes.push_back(parse_graph_node(item));
    }
  }
  if (node.contains("edges")) {
    if (!node.at("edges").is_array()) {
      throw std::runtime_error("graph edges must be an array");
    }
    for (const auto& item : node.at("edges")) {
      graph.edges.push_back(parse_graph_edge(item));
    }
  }
  return graph;
}

EventPage parse_page(const json& node) {
  EventPage page;
  page.trigger = parse_trigger(node.at("trigger").get<std::string>());
  if (node.contains("conditions")) {
    for (const auto& condition : node.at("conditions")) {
      page.conditions.push_back(parse_condition(condition));
    }
  }
  if (node.contains("commands")) {
    page.commands = parse_commands(node.at("commands"));
  }
  if (node.contains("graph")) {
    page.graph = parse_graph(node.at("graph"));
  }
  return page;
}

EventDef parse_event(const json& node) {
  EventDef event;
  event.id = node.at("id").get<std::string>();
  if (event.id.empty()) {
    throw std::runtime_error("event id must not be empty");
  }
  if (node.contains("tile")) {
    const auto& tile = node.at("tile");
    event.tile = TileCoord{tile.at("x").get<int>(), tile.at("z").get<int>()};
  }
  if (node.contains("volume")) {
    event.volume = parse_aabb(node.at("volume"));
  }
  if (!node.contains("pages") || !node.at("pages").is_array()) {
    throw std::runtime_error("event pages must be an array");
  }
  for (const auto& page : node.at("pages")) {
    event.pages.push_back(parse_page(page));
  }
  return event;
}

AssetKind parse_asset_kind(const std::string& value) {
  if (value == "texture") {
    return AssetKind::Texture;
  }
  if (value == "audio_clip") {
    return AssetKind::AudioClip;
  }
  if (value == "mesh") {
    return AssetKind::MeshDescriptor;
  }
  if (value == "material") {
    return AssetKind::MaterialDescriptor;
  }
  throw std::runtime_error("unknown asset kind: " + value);
}

MapAssetRef parse_map_asset(const json& node) {
  MapAssetRef ref;
  ref.id = make_asset_id(node.at("id").get<std::string>());
  if (node.contains("kind")) {
    ref.kind = parse_asset_kind(node.at("kind").get<std::string>());
  }
  ref.debug_name = node.value("debug_name", "");
  return ref;
}

MapData parse_map(const json& root) {
  MapData map;
  map.schema_version = root.at("schema_version").get<int>();
  if (map.schema_version != 1 && map.schema_version != 2 && map.schema_version != 3) {
    throw std::runtime_error("unsupported schema_version (expected 1, 2, or 3)");
  }
  map.id = root.at("id").get<std::string>();
  map.width = root.at("width").get<int>();
  map.height = root.at("height").get<int>();
  map.tile_size = root.value("tile_size", 1.0f);
  if (map.id.empty()) {
    throw std::runtime_error("map id must not be empty");
  }
  if (map.width <= 0 || map.height <= 0) {
    throw std::runtime_error("map width/height must be > 0");
  }
  if (map.schema_version == 1) {
    map.height_grid.origin_x = 0;
    map.height_grid.origin_z = 0;
    map.height_grid.width = map.width;
    map.height_grid.height = map.height;
    map.height_grid.ground_y.assign(static_cast<std::size_t>(map.width) * map.height, 0.0f);
  } else {
    if (!root.contains("height_grid") || !root.at("height_grid").is_object()) {
      throw std::runtime_error("schema v2 requires height_grid object");
    }
    const auto& grid = root.at("height_grid");
    map.height_grid.origin_x = grid.at("origin_x").get<int>();
    map.height_grid.origin_z = grid.at("origin_z").get<int>();
    map.height_grid.width = grid.at("width").get<int>();
    map.height_grid.height = grid.at("height").get<int>();
    if (map.height_grid.width <= 0 || map.height_grid.height <= 0) {
      throw std::runtime_error("height_grid width/height must be > 0");
    }
    map.height_grid.ground_y = grid.at("ground_y").get<std::vector<float>>();
    const std::size_t expected = static_cast<std::size_t>(map.height_grid.width) * map.height_grid.height;
    if (map.height_grid.ground_y.size() != expected) {
      throw std::runtime_error("height_grid ground_y length mismatch");
    }
    if (root.contains("ramps")) {
      for (const auto& ramp : root.at("ramps")) {
        RampDef out;
        const auto& tile = ramp.at("tile");
        out.tile = TileCoord{tile.at("x").get<int>(), tile.at("z").get<int>()};
        out.direction = parse_ramp_direction(ramp.at("direction").get<std::string>());
        out.low_y = ramp.at("low_y").get<float>();
        out.high_y = ramp.at("high_y").get<float>();
        if (out.high_y < out.low_y) {
          throw std::runtime_error("ramp high_y must be >= low_y");
        }
        map.ramps.push_back(out);
      }
    }
    if (root.contains("edge_barriers")) {
      for (const auto& node : root.at("edge_barriers")) {
        EdgeBarrierDef out;
        const auto& tile = node.at("tile");
        out.tile = TileCoord{tile.at("x").get<int>(), tile.at("z").get<int>()};
        out.direction = parse_ramp_direction(node.at("direction").get<std::string>());
        out.height = node.at("height").get<float>();
        map.edge_barriers.push_back(out);
      }
    }
    canonicalize_edge_barriers(map);
  }
  if (map.schema_version >= 3) {
    if (root.contains("floor_slabs")) {
      for (const auto& node : root.at("floor_slabs")) {
        FloorSlabDef out;
        const auto& tile = node.at("tile");
        out.tile = TileCoord{tile.at("x").get<int>(), tile.at("z").get<int>()};
        out.top_y = node.at("top_y").get<float>();
        out.thickness = node.value("thickness", kDefaultFloorSlabThickness);
        map.floor_slabs.push_back(out);
      }
    }
    if (root.contains("ladders")) {
      for (const auto& node : root.at("ladders")) {
        LadderDef out;
        const auto& tile = node.at("tile");
        out.tile = TileCoord{tile.at("x").get<int>(), tile.at("z").get<int>()};
        out.direction = parse_ramp_direction(node.at("direction").get<std::string>());
        out.y_lo = node.at("y_lo").get<float>();
        out.y_hi = node.at("y_hi").get<float>();
        map.ladders.push_back(out);
      }
    }
  }
  if (root.contains("blockers")) {
    for (const auto& blocker : root.at("blockers")) {
      map.blockers.push_back(parse_blocker(blocker));
    }
  }
  if (root.contains("events")) {
    for (const auto& event : root.at("events")) {
      map.events.push_back(parse_event(event));
    }
  }
  if (root.contains("assets")) {
    if (!root.at("assets").is_array()) {
      throw std::runtime_error("assets must be an array");
    }
    for (const auto& asset : root.at("assets")) {
      map.assets.push_back(parse_map_asset(asset));
    }
  }
  return map;
}

}  // namespace

MapLoadResult load_map_from_string(std::string_view json_text) {
  try {
    const json root = json::parse(json_text);
    MapLoadResult result;
    result.ok = true;
    result.map = parse_map(root);
    return result;
  } catch (const std::exception& ex) {
    return fail(ex.what());
  }
}

MapLoadResult load_map_from_file(const std::string& path) {
  return load_map_from_file(path, os_files());
}

MapLoadResult load_map_from_file(const std::string& path, const FileStore& files) {
  const FileReadResult read = files.read(path);
  if (!read.ok) {
    return fail(read.error.empty() ? "failed to open map file: " + path : read.error);
  }
  return load_map_from_string(read.bytes.as_text());
}

namespace {

using json = nlohmann::json;

const char* trigger_to_string(TriggerKind trigger) {
  switch (trigger) {
    case TriggerKind::Action:
      return "action";
    case TriggerKind::PlayerTouch:
      return "player_touch";
    case TriggerKind::EventTouch:
      return "event_touch";
    case TriggerKind::Autorun:
      return "autorun";
    case TriggerKind::Parallel:
      return "parallel";
  }
  return "action";
}

const char* compare_op_to_string(CompareOp op) {
  switch (op) {
    case CompareOp::Eq:
      return "==";
    case CompareOp::Ne:
      return "!=";
    case CompareOp::Lt:
      return "<";
    case CompareOp::Le:
      return "<=";
    case CompareOp::Gt:
      return ">";
    case CompareOp::Ge:
      return ">=";
  }
  return "==";
}

const char* ramp_direction_to_string(RampDirection dir) {
  switch (dir) {
    case RampDirection::North:
      return "north";
    case RampDirection::East:
      return "east";
    case RampDirection::South:
      return "south";
    case RampDirection::West:
      return "west";
  }
  return "north";
}

const char* asset_kind_to_string(AssetKind kind) {
  switch (kind) {
    case AssetKind::Texture:
      return "texture";
    case AssetKind::AudioClip:
      return "audio_clip";
    case AssetKind::MeshDescriptor:
      return "mesh";
    case AssetKind::MaterialDescriptor:
      return "material";
  }
  return "texture";
}

json dump_aabb(const Aabb2& box) {
  return json{{"min_x", box.min_x}, {"min_z", box.min_z}, {"max_x", box.max_x}, {"max_z", box.max_z}};
}

json dump_blocker(const BlockerDef& blocker) {
  json node = dump_aabb(blocker.bounds);
  if (blocker.base_y.has_value() || blocker.top_y.has_value() || blocker.jumpable) {
    if (blocker.base_y.has_value()) {
      node["base_y"] = *blocker.base_y;
    }
    if (blocker.top_y.has_value()) {
      node["top_y"] = *blocker.top_y;
    }
    node["jumpable"] = blocker.jumpable;
  }
  return node;
}

json dump_condition(const Condition& condition) {
  switch (condition.type) {
    case ConditionType::Switch:
      return json{{"type", "switch"}, {"id", condition.id}, {"value", condition.bool_value}};
    case ConditionType::Variable:
      return json{{"type", "variable"},
                  {"id", condition.id},
                  {"op", compare_op_to_string(condition.op)},
                  {"value", condition.int_value}};
    case ConditionType::Item:
      return json{{"type", "item"}, {"id", condition.string_id}, {"quantity", condition.int_value}};
    case ConditionType::SelfSwitch:
      return json{{"type", "self_switch"},
                  {"key", std::string(1, condition.self_switch)},
                  {"value", condition.bool_value}};
  }
  return json{{"type", "switch"}, {"id", 0}, {"value", false}};
}

json dump_commands(const std::vector<Command>& commands);

json dump_command(const Command& command) {
  switch (command.op) {
    case CommandOp::ShowText:
      return json{{"op", "show_text"}, {"text", command.text}};
    case CommandOp::ControlSwitch:
      return json{{"op", "control_switch"}, {"id", command.id}, {"value", command.bool_value}};
    case CommandOp::ControlVariable:
      return json{{"op", "control_variable"}, {"id", command.id}, {"value", command.int_value}};
    case CommandOp::ControlSelfSwitch:
      return json{{"op", "control_self_switch"},
                  {"key", std::string(1, command.self_switch)},
                  {"value", command.bool_value}};
    case CommandOp::ConditionalBranch: {
      json node{{"op", "conditional_branch"},
                {"condition", dump_condition(command.branch_condition)},
                {"then", dump_commands(command.then_commands)}};
      if (!command.else_commands.empty()) {
        node["else"] = dump_commands(command.else_commands);
      }
      return node;
    }
    case CommandOp::Wait:
      return json{{"op", "wait"}, {"frames", command.frames}};
    case CommandOp::TransferPlayer:
      return json{{"op", "transfer_player"},
                  {"map_id", command.map_id},
                  {"x", command.x},
                  {"y", command.y},
                  {"z", command.z}};
    case CommandOp::ChangeItems:
      return json{{"op", "change_items"},
                  {"id", command.item_id},
                  {"delta", command.item_delta},
                  {"key_item", command.key_item}};
    case CommandOp::PlaySE:
      return json{{"op", "play_se"}, {"id", command.text}};
    case CommandOp::Comment:
      return json{{"op", "comment"}, {"text", command.text}};
  }
  return json{{"op", "comment"}, {"text", ""}};
}

json dump_commands(const std::vector<Command>& commands) {
  json arr = json::array();
  for (const Command& command : commands) {
    arr.push_back(dump_command(command));
  }
  return arr;
}

json dump_graph_node(const EventGraphNode& node) {
  json params = json::object();
  if (node.kind == "show_text") {
    params["text"] = node.text;
  } else if (node.kind == "control_switch") {
    params["id"] = node.switch_id;
    params["value"] = node.bool_value;
  } else if (node.kind == "wait") {
    params["frames"] = node.frames;
  } else if (node.kind == "conditional_branch") {
    params["condition"] = dump_condition(node.branch_condition);
  } else {
    if (!node.text.empty()) {
      params["text"] = node.text;
    }
    if (node.switch_id != 0) {
      params["id"] = node.switch_id;
    }
    if (node.frames != 0) {
      params["frames"] = node.frames;
    }
  }
  return json{{"id", node.id}, {"kind", node.kind}, {"params", params}};
}

json dump_graph_edge(const EventGraphEdge& edge) {
  json node{{"from", edge.from}, {"to", edge.to}};
  if (edge.order.has_value()) {
    node["order"] = *edge.order;
  }
  if (edge.branch.has_value()) {
    node["branch"] = *edge.branch;
  }
  return node;
}

json dump_graph(const EventGraph& graph) {
  json node{{"nodes", json::array()}, {"edges", json::array()}};
  for (const EventGraphNode& graph_node : graph.nodes) {
    node["nodes"].push_back(dump_graph_node(graph_node));
  }
  for (const EventGraphEdge& edge : graph.edges) {
    node["edges"].push_back(dump_graph_edge(edge));
  }
  return node;
}

json dump_page(const EventPage& page) {
  json node{{"trigger", trigger_to_string(page.trigger)},
            {"conditions", json::array()},
            {"commands", dump_commands(page.commands)}};
  for (const Condition& condition : page.conditions) {
    node["conditions"].push_back(dump_condition(condition));
  }
  if (page.graph.has_value()) {
    node["graph"] = dump_graph(*page.graph);
  }
  return node;
}

json dump_event(const EventDef& event) {
  json node{{"id", event.id}, {"pages", json::array()}};
  if (event.tile.has_value()) {
    node["tile"] = json{{"x", event.tile->x}, {"z", event.tile->z}};
  }
  if (event.volume.has_value()) {
    node["volume"] = dump_aabb(*event.volume);
  }
  for (const EventPage& page : event.pages) {
    node["pages"].push_back(dump_page(page));
  }
  return node;
}

}  // namespace

MapSerializeResult serialize_map_to_string(const MapData& map) {
  try {
    json root{{"schema_version", map.schema_version},
              {"id", map.id},
              {"width", map.width},
              {"height", map.height},
              {"tile_size", map.tile_size},
              {"blockers", json::array()},
              {"events", json::array()}};
    if (map.schema_version >= 2) {
      root["height_grid"] = json{{"origin_x", map.height_grid.origin_x},
                                 {"origin_z", map.height_grid.origin_z},
                                 {"width", map.height_grid.width},
                                 {"height", map.height_grid.height},
                                 {"ground_y", map.height_grid.ground_y}};
      root["ramps"] = json::array();
      for (const RampDef& ramp : map.ramps) {
        root["ramps"].push_back(json{{"tile", {{"x", ramp.tile.x}, {"z", ramp.tile.z}}},
                                      {"direction", ramp_direction_to_string(ramp.direction)},
                                      {"low_y", ramp.low_y},
                                      {"high_y", ramp.high_y}});
      }
      root["edge_barriers"] = json::array();
      for (const EdgeBarrierDef& edge : map.edge_barriers) {
        root["edge_barriers"].push_back(json{{"tile", {{"x", edge.tile.x}, {"z", edge.tile.z}}},
                                             {"direction", ramp_direction_to_string(edge.direction)},
                                             {"height", edge.height}});
      }
    }
    if (map.schema_version >= 3) {
      root["floor_slabs"] = json::array();
      for (const FloorSlabDef& slab : map.floor_slabs) {
        root["floor_slabs"].push_back(json{{"tile", {{"x", slab.tile.x}, {"z", slab.tile.z}}},
                                            {"top_y", slab.top_y},
                                            {"thickness", slab.thickness}});
      }
      root["ladders"] = json::array();
      for (const LadderDef& ladder : map.ladders) {
        root["ladders"].push_back(json{{"tile", {{"x", ladder.tile.x}, {"z", ladder.tile.z}}},
                                       {"direction", ramp_direction_to_string(ladder.direction)},
                                       {"y_lo", ladder.y_lo},
                                       {"y_hi", ladder.y_hi}});
      }
    }
    for (const BlockerDef& blocker : map.blockers) {
      root["blockers"].push_back(dump_blocker(blocker));
    }
    for (const EventDef& event : map.events) {
      root["events"].push_back(dump_event(event));
    }
    if (!map.assets.empty()) {
      root["assets"] = json::array();
      for (const MapAssetRef& asset : map.assets) {
        json node{{"id", asset.id.key()}, {"kind", asset_kind_to_string(asset.kind)}};
        if (!asset.debug_name.empty()) {
          node["debug_name"] = asset.debug_name;
        }
        root["assets"].push_back(node);
      }
    }
    MapSerializeResult result;
    result.ok = true;
    result.json_text = root.dump(2);
    return result;
  } catch (const std::exception& ex) {
    MapSerializeResult result;
    result.ok = false;
    result.error = ex.what();
    return result;
  }
}

MapFileResult save_map_to_file(const MapData& map, const std::string& path) {
  return save_map_to_file(map, path, os_files());
}

MapFileResult save_map_to_file(const MapData& map, const std::string& path, FileStore& files) {
  const MapSerializeResult serialized = serialize_map_to_string(map);
  if (!serialized.ok) {
    return MapFileResult{false, serialized.error};
  }

  std::string payload = serialized.json_text;
  payload += '\n';
  const FileWriteResult written = files.write(path, payload);
  if (!written.ok) {
    return MapFileResult{false, written.error.empty() ? "failed to write map file: " + path
                                                      : written.error};
  }
  return MapFileResult{true, {}};
}

}  // namespace rat
