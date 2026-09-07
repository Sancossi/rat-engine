#pragma once

#include "rat/file_store.hpp"
#include "rat/simulation_session.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace rat {
inline constexpr std::uint32_t kReplaySchemaVersion = 2;
inline constexpr std::uint32_t kReplayRuntimeVersion = 2;
inline constexpr std::uint32_t kReplayChecksumVersion = 2;

enum class ReplayErrorCode { None, Io, Format, Incompatible, InvalidSession };
struct ReplayStatus {
  bool ok = false;
  ReplayErrorCode code = ReplayErrorCode::None;
  std::string error;
};
struct ReplayHeader {
  std::uint32_t schema_version = kReplaySchemaVersion;
  std::uint32_t runtime_version = kReplayRuntimeVersion;
  std::uint32_t checksum_version = kReplayChecksumVersion;
  std::string map_id;
  std::uint64_t map_fingerprint = 0;
  std::uint64_t seed = 0;
  SimulationConfig config{};
  PlayerBody start_player{};
};
struct TickInput {
  std::uint64_t tick_id = 0;
  InputFrame input{};
  std::uint64_t checksum = 0;
};
struct ReplayRecording { ReplayHeader header; std::vector<TickInput> ticks; };
struct ReplayRecordResult : ReplayStatus { ReplayRecording recording; };
struct ReplayPlayResult : ReplayStatus {
  std::uint64_t checksum = 0;
  bool checksums_match = false;
  std::optional<std::uint64_t> first_diverging_tick;
  std::uint64_t ticks_executed = 0;
  PlayerBody player{};
  GameState state{};
};

// Canonical bytes, not a digest: used for structural fresh-session comparisons.
[[nodiscard]] std::vector<std::uint8_t> runtime_state_bytes(const SimulationSession& session);
[[nodiscard]] std::uint64_t runtime_checksum(const SimulationSession& session, std::uint64_t seed);
[[nodiscard]] ReplayRecordResult begin_recording(const SimulationSession& session, std::uint64_t seed = 0);
[[nodiscard]] ReplayStatus record_tick(ReplayRecording& recording, std::uint64_t tick_id,
                                       const InputFrame& input, std::uint64_t checksum);
[[nodiscard]] ReplayRecordResult record_input_sequence(const MapData& map, const PlayerBody& start,
    std::span<const InputFrame> steps, std::uint64_t seed = 0, SimulationConfig config = {});
// Refuses incompatible or non-fresh sessions before running any tick.
[[nodiscard]] ReplayPlayResult play_recording(SimulationSession& session, const ReplayRecording& recording);
[[nodiscard]] ReplayPlayResult replay_input_sequence(const MapData& map, const ReplayRecording& recording);
[[nodiscard]] ReplayStatus validate_recording(const ReplayRecording& recording);
[[nodiscard]] ReplayStatus write_replay(std::string_view path, const ReplayRecording& recording);
[[nodiscard]] ReplayStatus write_replay(std::string_view path, const ReplayRecording& recording, FileStore& files);
[[nodiscard]] ReplayRecordResult read_replay(std::string_view path);
[[nodiscard]] ReplayRecordResult read_replay(std::string_view path, const FileStore& files);
} // namespace rat
