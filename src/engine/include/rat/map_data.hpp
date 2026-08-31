#pragma once

#include "rat/player.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rat {

enum class TriggerKind {
  Action,
  PlayerTouch,
  EventTouch,
  Autorun,
  Parallel,
};

enum class ConditionType {
  Switch,
  Variable,
  Item,
  SelfSwitch,
};

enum class CompareOp {
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
};

enum class CommandOp {
  ShowText,
  ControlSwitch,
  ControlVariable,
  ControlSelfSwitch,
  ConditionalBranch,
  Wait,
  TransferPlayer,
  ChangeItems,
  Comment,
};

struct TileCoord {
  int x = 0;
  int z = 0;
};

struct Condition {
  ConditionType type = ConditionType::Switch;
  std::uint32_t id = 0;
  bool bool_value = true;
  int int_value = 0;
  CompareOp op = CompareOp::Eq;
  std::string string_id;
  char self_switch = 'A';
};

struct Command {
  CommandOp op = CommandOp::Comment;
  std::string text;
  std::uint32_t id = 0;
  bool bool_value = false;
  int int_value = 0;
  int frames = 0;
  std::string map_id;
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  std::string item_id;
  int item_delta = 0;
  bool key_item = false;
  char self_switch = 'A';
  Condition branch_condition{};
  std::vector<Command> then_commands;
  std::vector<Command> else_commands;
};

struct EventPage {
  TriggerKind trigger = TriggerKind::Action;
  std::vector<Condition> conditions;
  std::vector<Command> commands;
};

struct EventDef {
  std::string id;
  std::optional<TileCoord> tile;
  std::optional<Aabb2> volume;
  std::vector<EventPage> pages;
};

struct MapData {
  int schema_version = 1;
  std::string id;
  int width = 0;
  int height = 0;
  float tile_size = 1.0f;
  std::vector<Aabb2> blockers;
  std::vector<EventDef> events;
};

}  // namespace rat
