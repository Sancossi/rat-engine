#pragma once

#include "rat/app_mode.hpp"
#include "rat/game_state.hpp"
#include "rat/input.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace rat {

struct InputSequenceConfig {
  float dt = 1.0f / 120.0f;
  JumpTuning jump_tuning{};
  // Default Play. Edit skips player control and snaps to ground each tick.
  AppMode app_mode = AppMode::Play;
  bool write_snapshot_each_step = false;
  std::string snapshot_dir;
};

struct InputSequenceResult {
  PlayerBody player;
  JumpState jump;
  GameState state;
  std::uint64_t sim_frame = 0;
  std::uint64_t snapshots_written = 0;
  std::optional<std::string> snapshot_error;
};

[[nodiscard]] InputSequenceResult run_input_sequence(
    const MapData& map, PlayerBody start_player, std::span<const InputFrame> steps,
    const InputSequenceConfig& config = {});

}  // namespace rat
