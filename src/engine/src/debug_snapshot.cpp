#include "rat/debug_snapshot.hpp"

#include "rat/app_mode.hpp"
#include "rat/asset.hpp"
#include "rat/audio.hpp"
#include "rat/collision.hpp"
#include "rat/file_store.hpp"
#include "rat/render_world.hpp"
#include "rat/simulation_session.hpp"
#include "rat/surface_query.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>

namespace rat {

using json = nlohmann::json;

namespace {

json dump_jump(const JumpState& jump) {
  return json{{"jump_offset", jump.jump_offset},
              {"vertical_speed", jump.vertical_speed},
              {"coyote_time_left", jump.coyote_time_left},
              {"jump_buffer_left", jump.jump_buffer_left},
              {"grounded", jump.grounded},
              {"support_blocker_index", jump.support_blocker_index}};
}

JumpState load_jump(const json& node) {
  JumpState jump = make_grounded_jump_state();
  if (!node.is_object()) {
    return jump;
  }
  jump.jump_offset = node.value("jump_offset", 0.0f);
  jump.vertical_speed = node.value("vertical_speed", 0.0f);
  jump.coyote_time_left = node.value("coyote_time_left", 0.0f);
  jump.jump_buffer_left = node.value("jump_buffer_left", 0.0f);
  jump.grounded = node.value("grounded", true);
  jump.support_blocker_index = node.value("support_blocker_index", -1);
  return jump;
}

json dump_interpreter(const DebugInterpreter& interp) {
  return json{{"event_id", interp.event_id},
              {"page_index", interp.page_index},
              {"command_index", interp.command_index},
              {"wait_frames", interp.wait_frames},
              {"waiting_message", interp.waiting_message}};
}

std::optional<DebugInterpreter> load_interpreter(const json& node) {
  if (node.is_null() || !node.is_object()) {
    return std::nullopt;
  }
  DebugInterpreter interp;
  interp.event_id = node.value("event_id", std::string{});
  interp.page_index = node.value("page_index", -1);
  interp.command_index = node.value("command_index", -1);
  interp.wait_frames = node.value("wait_frames", 0);
  interp.waiting_message = node.value("waiting_message", false);
  return interp;
}

json dump_snapshot(const DebugSnapshot& snapshot) {
  json switches = json::object();
  for (const auto& [id, value] : snapshot.switches) {
    switches[std::to_string(id)] = value;
  }
  json variables = json::object();
  for (const auto& [id, value] : snapshot.variables) {
    variables[std::to_string(id)] = value;
  }
  json items = json::array();
  for (const InventoryItem& item : snapshot.items) {
    items.push_back(json{{"id", item.id}, {"quantity", item.quantity}, {"key_item", item.key_item}});
  }

  json root;
  root["sim_frame"] = snapshot.sim_frame;
  root["app_mode"] = snapshot.app_mode;
  root["player"] = json{{"x", snapshot.player_x}, {"y", snapshot.player_y}, {"z", snapshot.player_z}};
  root["jump"] = dump_jump(snapshot.jump);
  root["overlapping_event_ids"] = snapshot.overlapping_event_ids;
  if (snapshot.active_interpreter.has_value()) {
    root["active_interpreter"] = dump_interpreter(*snapshot.active_interpreter);
  } else {
    root["active_interpreter"] = nullptr;
  }
  root["game_state"] = json{{"switches", switches}, {"variables", variables}, {"items", items}};
  if (snapshot.active_message.has_value()) {
    root["active_message"] = *snapshot.active_message;
  } else {
    root["active_message"] = nullptr;
  }
  root["warnings"] = snapshot.warnings;
  root["event_why_not_reason"] = snapshot.event_why_not_reason;
  json why_not = json::array();
  for (const EventWhyNotEntry& entry : snapshot.event_why_not) {
    why_not.push_back(json{{"id", entry.id}, {"reason", entry.reason}});
  }
  root["event_why_not"] = why_not;
  root["checksum"] = snapshot.checksum;
  root["input"] = json{{"axis_x", snapshot.input.move.axis_x},
                       {"axis_z", snapshot.input.move.axis_z},
                       {"jump_pressed", snapshot.input.jump_pressed},
                       {"jump_held", snapshot.input.jump_held},
                       {"interact_pressed", snapshot.input.interact_pressed}};
  root["metrics"] = json{{"simulation_tick_seconds", snapshot.metrics.simulation_tick_seconds},
                         {"event_commands", snapshot.metrics.event_commands},
                         {"draw_calls", snapshot.metrics.draw_calls},
                         {"transient_bytes", snapshot.metrics.transient_bytes},
                         {"transient_allocations", snapshot.metrics.transient_allocations},
                         {"asset_uploads", snapshot.metrics.asset_uploads},
                         {"audio_queue_depth", snapshot.metrics.audio_queue_depth},
                         {"audio_overflow_count", snapshot.metrics.audio_overflow_count},
                         {"collision_candidates", snapshot.metrics.collision_candidates}};
  return root;
}

DebugSnapshot load_snapshot(const json& root) {
  DebugSnapshot snapshot;
  snapshot.sim_frame = root.value("sim_frame", static_cast<std::uint64_t>(0));
  snapshot.app_mode = root.value("app_mode", std::string{});
  if (root.contains("player") && root["player"].is_object()) {
    snapshot.player_x = root["player"].value("x", 0.0f);
    snapshot.player_y = root["player"].value("y", 0.0f);
    snapshot.player_z = root["player"].value("z", 0.0f);
  }
  if (root.contains("jump")) {
    snapshot.jump = load_jump(root["jump"]);
  }
  if (root.contains("overlapping_event_ids") && root["overlapping_event_ids"].is_array()) {
    snapshot.overlapping_event_ids = root["overlapping_event_ids"].get<std::vector<std::string>>();
  }
  if (root.contains("active_interpreter")) {
    snapshot.active_interpreter = load_interpreter(root["active_interpreter"]);
  }
  if (root.contains("game_state") && root["game_state"].is_object()) {
    const json& gs = root["game_state"];
    if (gs.contains("switches") && gs["switches"].is_object()) {
      for (auto it = gs["switches"].begin(); it != gs["switches"].end(); ++it) {
        snapshot.switches[static_cast<std::uint32_t>(std::stoul(it.key()))] = it.value().get<bool>();
      }
    }
    if (gs.contains("variables") && gs["variables"].is_object()) {
      for (auto it = gs["variables"].begin(); it != gs["variables"].end(); ++it) {
        snapshot.variables[static_cast<std::uint32_t>(std::stoul(it.key()))] = it.value().get<int>();
      }
    }
    if (gs.contains("items") && gs["items"].is_array()) {
      for (const json& item : gs["items"]) {
        snapshot.items.push_back(InventoryItem{
            item.value("id", std::string{}),
            item.value("quantity", 0),
            item.value("key_item", false),
        });
      }
    }
  }
  if (root.contains("active_message") && root["active_message"].is_string()) {
    snapshot.active_message = root["active_message"].get<std::string>();
  }
  if (root.contains("warnings") && root["warnings"].is_array()) {
    snapshot.warnings = root["warnings"].get<std::vector<std::string>>();
  }
  snapshot.event_why_not_reason = root.value("event_why_not_reason", std::string{});
  if (root.contains("event_why_not") && root["event_why_not"].is_array()) {
    for (const json& item : root["event_why_not"]) {
      if (!item.is_object()) {
        continue;
      }
      snapshot.event_why_not.push_back(EventWhyNotEntry{
          item.value("id", std::string{}),
          item.value("reason", std::string{}),
      });
    }
  }
  snapshot.checksum = root.value("checksum", static_cast<std::uint64_t>(0));
  if (root.contains("input") && root["input"].is_object()) {
    const json& input = root["input"];
    snapshot.input.move.axis_x = input.value("axis_x", 0.0f);
    snapshot.input.move.axis_z = input.value("axis_z", 0.0f);
    snapshot.input.jump_pressed = input.value("jump_pressed", false);
    snapshot.input.jump_held = input.value("jump_held", false);
    snapshot.input.interact_pressed = input.value("interact_pressed", false);
  }
  if (root.contains("metrics") && root["metrics"].is_object()) {
    const json& metrics = root["metrics"];
    snapshot.metrics.simulation_tick_seconds = metrics.value("simulation_tick_seconds", 0.0);
    snapshot.metrics.event_commands = metrics.value("event_commands", 0);
    snapshot.metrics.draw_calls = metrics.value("draw_calls", 0);
    snapshot.metrics.transient_bytes = metrics.value("transient_bytes", static_cast<std::uint64_t>(0));
    snapshot.metrics.transient_allocations = metrics.value("transient_allocations", 0);
    snapshot.metrics.asset_uploads = metrics.value("asset_uploads", 0);
    snapshot.metrics.audio_queue_depth = metrics.value("audio_queue_depth", 0);
    snapshot.metrics.audio_overflow_count =
        metrics.value("audio_overflow_count", static_cast<std::uint64_t>(0));
    snapshot.metrics.collision_candidates = metrics.value("collision_candidates", 0);
  }
  return snapshot;
}

}  // namespace

DebugSnapshot make_debug_snapshot(std::uint64_t sim_frame, AppMode mode, const PlayerBody& player,
                                  const JumpState& jump, const EventRuntime& events,
                                  const GameState& state, bool interact_pressed,
                                  std::string_view selected_event_id, const InputFrame& input,
                                  std::uint64_t checksum, FrameMetrics metrics) {
  DebugSnapshot snapshot;
  snapshot.sim_frame = sim_frame;
  snapshot.app_mode = app_mode_name(mode);
  snapshot.player_x = player.x;
  snapshot.player_y = player.y;
  snapshot.player_z = player.z;
  snapshot.jump = jump;
  snapshot.overlapping_event_ids = events.overlapping_event_ids(player);
  if (const auto foreground = events.foreground_debug()) {
    snapshot.active_interpreter = DebugInterpreter{
        foreground->event_id,
        foreground->page_index,
        foreground->command_index,
        foreground->wait_frames,
        foreground->waiting_message,
    };
  }
  snapshot.switches = state.debug_switches();
  snapshot.variables = state.debug_variables();
  snapshot.items = state.inventory();
  snapshot.active_message = events.active_message();
  snapshot.warnings = events.warnings();
  for (const EventDef& event : events.map().events) {
    const EventWhyNot reason =
        events.why_not_fired(event.id, state, player, interact_pressed);
    snapshot.event_why_not.push_back(EventWhyNotEntry{event.id, event_why_not_name(reason)});
  }

  const auto set_reason_for_id = [&](std::string_view id) -> bool {
    for (const EventWhyNotEntry& entry : snapshot.event_why_not) {
      if (entry.id == id) {
        snapshot.event_why_not_reason = entry.reason;
        return true;
      }
    }
    return false;
  };

  if (selected_event_id.empty() || !set_reason_for_id(selected_event_id)) {
    if (!snapshot.overlapping_event_ids.empty()) {
      set_reason_for_id(snapshot.overlapping_event_ids.front());
    } else if (!snapshot.event_why_not.empty()) {
      snapshot.event_why_not_reason = snapshot.event_why_not.front().reason;
    }
  }
  snapshot.input = input;
  snapshot.checksum = checksum;
  snapshot.metrics = metrics;
  return snapshot;
}

bool write_debug_snapshot(std::string_view path, const DebugSnapshot& snapshot) {
  return write_debug_snapshot(path, snapshot, os_files());
}

bool write_debug_snapshot(std::string_view path, const DebugSnapshot& snapshot, FileStore& files) {
  const std::string json_text = dump_snapshot(snapshot).dump(2) + '\n';
  return files.write(path, json_text).ok;
}

std::optional<DebugSnapshot> read_debug_snapshot(std::string_view path) {
  return read_debug_snapshot(path, os_files());
}

std::optional<DebugSnapshot> read_debug_snapshot(std::string_view path, const FileStore& files) {
  const FileReadResult read = files.read(path);
  if (!read.ok) {
    return std::nullopt;
  }
  try {
    const json root = json::parse(read.bytes.as_text());
    return load_snapshot(root);
  } catch (...) {
    return std::nullopt;
  }
}

std::string default_debug_snapshot_path() {
  return "rat-debug.json";
}

FrameMetrics collect_frame_metrics(const FrameMetricsSources& sources) {
  FrameMetrics metrics;
  if (sources.session != nullptr) {
    metrics.simulation_tick_seconds = sources.session->last_tick_seconds();
    metrics.event_commands = sources.session->events().last_commands_executed();
    if (const SurfaceQuery* surface = sources.session->surface(); surface != nullptr) {
      const CollisionWorld world =
          bake_collision_world(sources.session->events().map(), *surface);
      metrics.collision_candidates = static_cast<int>(world.fences.size());
    }
  }
  if (sources.render != nullptr) {
    metrics.draw_calls = static_cast<int>(sources.render->packets.size());
  }
  if (sources.frame != nullptr) {
    metrics.transient_bytes = sources.frame->used();
    metrics.transient_allocations = static_cast<int>(sources.frame->allocation_count());
  }
  if (sources.assets != nullptr) {
    metrics.asset_uploads = sources.assets->last_gpu_uploads();
  }
  if (sources.audio != nullptr) {
    metrics.audio_queue_depth = static_cast<int>(sources.audio->queue_depth());
    metrics.audio_overflow_count = sources.audio->overflow_count();
  }
  return metrics;
}

}  // namespace rat
