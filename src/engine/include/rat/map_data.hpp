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
  PlaySE,
  Comment,
};

struct TileCoord {
  int x = 0;
  int z = 0;
};

enum class RampDirection {
  North,
  East,
  South,
  West,
};

struct HeightGrid {
  int origin_x = 0;
  int origin_z = 0;
  int width = 0;
  int height = 0;
  std::vector<float> ground_y;
};

struct RampDef {
  TileCoord tile;
  RampDirection direction = RampDirection::North;
  float low_y = 0.0f;
  float high_y = 0.0f;
};

struct BlockerDef {
  Aabb2 bounds{};
  std::optional<float> base_y{};
  std::optional<float> top_y{};
  bool jumpable = false;
};

[[nodiscard]] inline bool blocker_blocks_feet(const BlockerDef& blocker, float feet_world_y,
                                              float epsilon = 1e-4f) {
  if (!blocker.jumpable) {
    return true;
  }
  if (!blocker.base_y.has_value() || !blocker.top_y.has_value()) {
    return true;
  }
  const float base = *blocker.base_y;
  const float top = *blocker.top_y;
  if (feet_world_y + epsilon < base) {
    return false;
  }
  if (feet_world_y >= top - epsilon) {
    return false;
  }
  return true;
}

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
  HeightGrid height_grid;
  std::vector<RampDef> ramps;
  std::vector<BlockerDef> blockers;
  std::vector<EventDef> events;
};

}  // namespace rat
