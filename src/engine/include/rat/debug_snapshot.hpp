#pragma once

#include "rat/app_mode.hpp"
#include "rat/event_runtime.hpp"
#include "rat/game_state.hpp"
#include "rat/input.hpp"
#include "rat/player.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

struct DebugInterpreter {
  std::string event_id;
  int page_index = -1;
  int command_index = -1;
  int wait_frames = 0;
  bool waiting_message = false;
};

struct EventWhyNotEntry {
  std::string id;
  std::string reason;
};

struct DebugSnapshot {
  std::uint64_t sim_frame = 0;
  std::string app_mode;
  float player_x = 0.0f;
  float player_y = 0.0f;
  float player_z = 0.0f;
  JumpState jump{};
  std::vector<std::string> overlapping_event_ids;
  std::optional<DebugInterpreter> active_interpreter;
  std::map<std::uint32_t, bool> switches;
  std::map<std::uint32_t, int> variables;
  std::vector<InventoryItem> items;
  std::optional<std::string> active_message;
  std::vector<std::string> warnings;
  std::string event_why_not_reason;
  std::vector<EventWhyNotEntry> event_why_not;
  std::uint64_t checksum = 0;
  InputFrame input{};
};

[[nodiscard]] DebugSnapshot make_debug_snapshot(std::uint64_t sim_frame, AppMode mode,
                                                const PlayerBody& player, const JumpState& jump,
                                                const EventRuntime& events, const GameState& state,
                                                bool interact_pressed = false,
                                                std::string_view selected_event_id = {},
                                                const InputFrame& input = {},
                                                std::uint64_t checksum = 0);

[[nodiscard]] bool write_debug_snapshot(std::string_view path, const DebugSnapshot& snapshot);
[[nodiscard]] std::optional<DebugSnapshot> read_debug_snapshot(std::string_view path);
[[nodiscard]] std::string default_debug_snapshot_path();

}  // namespace rat
