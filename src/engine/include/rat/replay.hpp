#pragma once

#include "rat/file_store.hpp"
#include "rat/game_state.hpp"
#include "rat/input.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/simulation_session.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

inline constexpr std::uint32_t kReplaySchemaVersion = 1;

struct ReplayHeader {
  std::uint32_t schema_version = kReplaySchemaVersion;
  std::string map_id;
  std::uint64_t seed = 0;
  float dt = kSimulationFixedDt;
  PlayerBody start_player{};
};

struct TickInput {
  std::uint64_t tick_id = 0;
  InputFrame input{};
  std::uint64_t checksum = 0;
};

struct ReplayRecording {
  ReplayHeader header;
  std::vector<TickInput> ticks;
};

struct ReplayPlayResult {
  std::uint64_t checksum = 0;
  bool checksums_match = true;
  std::optional<std::uint64_t> first_diverging_tick;
  PlayerBody player{};
  GameState state{};
};

[[nodiscard]] std::uint64_t runtime_checksum(const SimulationSession& session, std::uint64_t seed);

void record_tick(ReplayRecording& recording, std::uint64_t tick_id, const InputFrame& input,
                 std::uint64_t checksum);

[[nodiscard]] ReplayRecording record_input_sequence(const MapData& map, const PlayerBody& start,
                                                    std::span<const InputFrame> steps,
                                                    std::uint64_t seed = 0);

[[nodiscard]] ReplayPlayResult play_recording(SimulationSession& session,
                                              const ReplayRecording& recording);

[[nodiscard]] ReplayPlayResult replay_input_sequence(const MapData& map,
                                                     const ReplayRecording& recording);

[[nodiscard]] bool write_replay(std::string_view path, const ReplayRecording& recording);
[[nodiscard]] bool write_replay(std::string_view path, const ReplayRecording& recording,
                                FileStore& files);
[[nodiscard]] std::optional<ReplayRecording> read_replay(std::string_view path);
[[nodiscard]] std::optional<ReplayRecording> read_replay(std::string_view path,
                                                         const FileStore& files);

}  // namespace rat
