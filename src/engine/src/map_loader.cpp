#include "rat/map_loader.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

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

MapData parse_map(const json& root) {
  MapData map;
  map.schema_version = root.at("schema_version").get<int>();
  if (map.schema_version != 1) {
    throw std::runtime_error("unsupported schema_version (expected 1)");
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
  if (root.contains("blockers")) {
    for (const auto& blocker : root.at("blockers")) {
      map.blockers.push_back(parse_aabb(blocker));
    }
  }
  if (root.contains("events")) {
    for (const auto& event : root.at("events")) {
      map.events.push_back(parse_event(event));
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
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return fail("failed to open map file: " + path);
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  return load_map_from_string(oss.str());
}

}  // namespace rat
