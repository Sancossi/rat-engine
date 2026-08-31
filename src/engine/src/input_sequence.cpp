#include "rat/input_sequence.hpp"

#include "rat/debug_snapshot.hpp"
#include "rat/event_runtime.hpp"
#include "rat/surface_query.hpp"

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
  result.player = start_player;
  result.jump = make_grounded_jump_state();
  result.jump.coyote_time_left = config.jump_tuning.coyote_seconds;
  result.state.set_map_id(map.id);
  result.state.set_player_position(result.player.x, result.player.y, result.player.z);

  EventRuntime events;
  events.load(map);

  const bool want_snapshots = config.write_snapshot_each_step && !config.snapshot_dir.empty();
  if (want_snapshots) {
    std::error_code ec;
    std::filesystem::create_directories(config.snapshot_dir, ec);
    if (ec) {
      result.snapshot_error = "failed to create snapshot_dir: " + ec.message();
    }
  }

  SurfaceQuery surface(map);
  const float dt = config.dt;

  for (const InputFrame& frame : steps) {
    ++result.sim_frame;

    const bool control = player_control_enabled(config.app_mode);
    const bool events_on = event_runtime_enabled(config.app_mode);
    const bool blocked = events.player_input_blocked();

    if (!control || blocked) {
      result.jump.jump_buffer_left = 0.0f;
    }

    if (control) {
      PlayerFrameInput frame_input = player_input_from_frame(frame);
      if (blocked) {
        frame_input.move = {};
        frame_input.jump_pressed = false;
        frame_input.jump_held = false;
      }
      const PlayerFrameResult integrated = integrate_player_frame_surface(
          result.player, result.jump, frame_input, dt, map.blockers, surface, config.jump_tuning,
          0.35f, map.edge_barriers);
      result.player = integrated.body;
      result.jump = integrated.jump;
      result.state.set_player_position(result.player.x, result.player.y, result.player.z);
    }

    if (events_on) {
      bool gameplay_interact = false;
      if (frame.interact_pressed && events.active_message().has_value()) {
        events.acknowledge_message();
      } else if (frame.interact_pressed && !events.player_input_blocked()) {
        gameplay_interact = true;
      }

      const std::string before_map_id = result.state.map_id();
      const float before_x = result.state.player_x();
      const float before_y = result.state.player_y();
      const float before_z = result.state.player_z();
      events.update(result.state, result.player, gameplay_interact, dt);

      const bool transferred = result.state.map_id() != before_map_id ||
                               result.state.player_x() != before_x ||
                               result.state.player_y() != before_y ||
                               result.state.player_z() != before_z;
      result.player.x = result.state.player_x();
      result.player.y = result.state.player_y();
      result.player.z = result.state.player_z();
      if (transferred) {
        result.jump = make_grounded_jump_state();
        result.jump.coyote_time_left = config.jump_tuning.coyote_seconds;
        result.jump.jump_buffer_left = 0.0f;
      }
    }

    if (want_snapshots && !result.snapshot_error) {
      const DebugSnapshot snapshot =
          make_debug_snapshot(result.sim_frame, config.app_mode, result.player, result.jump, events,
                              result.state, frame.interact_pressed);
      const std::string path = snapshot_step_path(config.snapshot_dir, result.sim_frame);
      if (write_debug_snapshot(path, snapshot)) {
        ++result.snapshots_written;
      } else {
        result.snapshot_error = "failed to write snapshot: " + path;
      }
    }
  }

  return result;
}

}  // namespace rat
