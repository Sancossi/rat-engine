#include "rat/input_sequence.hpp"

#include "rat/debug_snapshot.hpp"
#include "rat/replay.hpp"
#include "rat/simulation_session.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

namespace rat {
namespace {

[[nodiscard]] std::string snapshot_step_path(const std::string& dir, std::uint64_t frame) {
  char name[32];
  std::snprintf(name, sizeof(name), "step-%06llu.json", static_cast<unsigned long long>(frame));
  return (std::filesystem::path(dir) / name).string();
}

}  // namespace

InputSequenceResult run_input_sequence(const MapData& map, PlayerBody start_player,
                                       std::span<const InputFrame> steps,
                                       const InputSequenceConfig& config) {
  InputSequenceResult result;

  SimulationConfig sim_config;
  sim_config.dt = config.dt;
  sim_config.jump_tuning = config.jump_tuning;
  sim_config.app_mode = config.app_mode;

  SimulationSession session(sim_config);
  session.load(map);
  session.set_player(start_player);

  const bool want_snapshots = config.write_snapshot_each_step && !config.snapshot_dir.empty();
  if (want_snapshots) {
    std::error_code ec;
    std::filesystem::create_directories(config.snapshot_dir, ec);
    if (ec) {
      result.snapshot_error = "failed to create snapshot_dir: " + ec.message();
    }
  }

  for (const InputFrame& frame : steps) {
    session.tick(frame);

    if (want_snapshots && !result.snapshot_error) {
      const DebugSnapshot snapshot = make_debug_snapshot(
          session.tick_id(), config.app_mode, session.player(), session.jump(), session.events(),
          session.state(), frame.interact_pressed, {}, frame,
          runtime_checksum(session, 0));
      const std::string path = snapshot_step_path(config.snapshot_dir, session.tick_id());
      if (write_debug_snapshot(path, snapshot)) {
        ++result.snapshots_written;
      } else {
        result.snapshot_error = "failed to write snapshot: " + path;
      }
    }
  }

  result.player = session.player();
  result.jump = session.jump();
  result.state = session.state();
  result.sim_frame = session.tick_id();
  return result;
}

}  // namespace rat
