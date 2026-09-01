#include "rat/replay.hpp"

#include "rat/file_store.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace rat {

using json = nlohmann::json;

namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

void mix_bytes(std::uint64_t& hash, const void* data, std::size_t size) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= kFnvPrime;
  }
}

void mix_u64(std::uint64_t& hash, std::uint64_t value) {
  mix_bytes(hash, &value, sizeof(value));
}

void mix_u32(std::uint64_t& hash, std::uint32_t value) {
  mix_bytes(hash, &value, sizeof(value));
}

void mix_i32(std::uint64_t& hash, std::int32_t value) {
  mix_bytes(hash, &value, sizeof(value));
}

void mix_f32(std::uint64_t& hash, float value) {
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  mix_u32(hash, bits);
}

void mix_bool(std::uint64_t& hash, bool value) {
  const unsigned char bit = value ? 1 : 0;
  mix_bytes(hash, &bit, 1);
}

void mix_string(std::uint64_t& hash, std::string_view value) {
  mix_u64(hash, value.size());
  mix_bytes(hash, value.data(), value.size());
}

void mix_player(std::uint64_t& hash, const PlayerBody& player) {
  mix_f32(hash, player.x);
  mix_f32(hash, player.y);
  mix_f32(hash, player.z);
  mix_f32(hash, player.half_extent);
  mix_f32(hash, player.speed);
}

void mix_jump(std::uint64_t& hash, const JumpState& jump) {
  mix_f32(hash, jump.jump_offset);
  mix_f32(hash, jump.vertical_speed);
  mix_f32(hash, jump.coyote_time_left);
  mix_f32(hash, jump.jump_buffer_left);
  mix_bool(hash, jump.grounded);
  mix_i32(hash, jump.support_blocker_index);
}

void mix_interpreter(std::uint64_t& hash, const InterpreterDebug& interp) {
  mix_string(hash, interp.event_id);
  mix_i32(hash, interp.page_index);
  mix_i32(hash, interp.command_index);
  mix_i32(hash, interp.wait_frames);
  mix_bool(hash, interp.waiting_message);
  mix_bool(hash, interp.parallel);
}

json dump_input(const InputFrame& input) {
  return json{{"axis_x", input.move.axis_x},
              {"axis_z", input.move.axis_z},
              {"jump_pressed", input.jump_pressed},
              {"jump_held", input.jump_held},
              {"interact_pressed", input.interact_pressed}};
}

InputFrame load_input(const json& node) {
  InputFrame input;
  if (!node.is_object()) {
    return input;
  }
  input.move.axis_x = node.value("axis_x", 0.0f);
  input.move.axis_z = node.value("axis_z", 0.0f);
  input.jump_pressed = node.value("jump_pressed", false);
  input.jump_held = node.value("jump_held", false);
  input.interact_pressed = node.value("interact_pressed", false);
  return input;
}

json dump_player(const PlayerBody& player) {
  return json{{"x", player.x},
              {"y", player.y},
              {"z", player.z},
              {"half_extent", player.half_extent},
              {"speed", player.speed}};
}

PlayerBody load_player(const json& node) {
  PlayerBody player;
  if (!node.is_object()) {
    return player;
  }
  player.x = node.value("x", 0.0f);
  player.y = node.value("y", 0.0f);
  player.z = node.value("z", 0.0f);
  player.half_extent = node.value("half_extent", 0.4f);
  player.speed = node.value("speed", 5.0f);
  return player;
}

json dump_recording(const ReplayRecording& recording) {
  json ticks = json::array();
  for (const TickInput& tick : recording.ticks) {
    ticks.push_back(json{{"tick_id", tick.tick_id},
                         {"checksum", tick.checksum},
                         {"input", dump_input(tick.input)}});
  }
  return json{
      {"schema_version", recording.header.schema_version},
      {"header",
       json{{"map_id", recording.header.map_id},
            {"seed", recording.header.seed},
            {"dt", recording.header.dt},
            {"start_player", dump_player(recording.header.start_player)}}},
      {"ticks", std::move(ticks)},
  };
}

ReplayRecording load_recording(const json& root) {
  ReplayRecording recording;
  recording.header.schema_version = root.value("schema_version", kReplaySchemaVersion);
  if (root.contains("header") && root["header"].is_object()) {
    const json& header = root["header"];
    recording.header.map_id = header.value("map_id", std::string{});
    recording.header.seed = header.value("seed", static_cast<std::uint64_t>(0));
    recording.header.dt = header.value("dt", kSimulationFixedDt);
    if (header.contains("start_player")) {
      recording.header.start_player = load_player(header["start_player"]);
    }
  }
  if (root.contains("ticks") && root["ticks"].is_array()) {
    for (const json& node : root["ticks"]) {
      if (!node.is_object()) {
        continue;
      }
      TickInput tick;
      tick.tick_id = node.value("tick_id", static_cast<std::uint64_t>(0));
      tick.checksum = node.value("checksum", static_cast<std::uint64_t>(0));
      if (node.contains("input")) {
        tick.input = load_input(node["input"]);
      }
      recording.ticks.push_back(tick);
    }
  }
  return recording;
}

}  // namespace

std::uint64_t runtime_checksum(const SimulationSession& session, std::uint64_t seed) {
  std::uint64_t hash = kFnvOffset;
  mix_u64(hash, seed);
  mix_u64(hash, session.tick_id());
  mix_i32(hash, static_cast<std::int32_t>(session.config().app_mode));
  mix_player(hash, session.player());
  mix_jump(hash, session.jump());

  const GameState& state = session.state();
  mix_string(hash, state.map_id());
  mix_f32(hash, state.player_x());
  mix_f32(hash, state.player_y());
  mix_f32(hash, state.player_z());

  for (const auto& [id, value] : state.debug_switches()) {
    mix_u32(hash, id);
    mix_bool(hash, value);
  }
  for (const auto& [id, value] : state.debug_variables()) {
    mix_u32(hash, id);
    mix_i32(hash, value);
  }
  for (const InventoryItem& item : state.inventory()) {
    mix_string(hash, item.id);
    mix_i32(hash, item.quantity);
    mix_bool(hash, item.key_item);
  }
  for (const EventDef& event : session.events().map().events) {
    mix_string(hash, event.id);
    mix_bool(hash, state.get_self_switch(event.id, 'A'));
    mix_bool(hash, state.get_self_switch(event.id, 'B'));
    mix_bool(hash, state.get_self_switch(event.id, 'C'));
    mix_bool(hash, state.get_self_switch(event.id, 'D'));
  }

  const EventRuntime& events = session.events();
  if (events.active_message().has_value()) {
    mix_bool(hash, true);
    mix_string(hash, *events.active_message());
  } else {
    mix_bool(hash, false);
  }
  if (const auto foreground = events.foreground_debug()) {
    mix_bool(hash, true);
    mix_interpreter(hash, *foreground);
  } else {
    mix_bool(hash, false);
  }
  const std::vector<InterpreterDebug> parallels = events.parallel_debug();
  mix_u64(hash, parallels.size());
  for (const InterpreterDebug& interp : parallels) {
    mix_interpreter(hash, interp);
  }
  return hash;
}

void record_tick(ReplayRecording& recording, std::uint64_t tick_id, const InputFrame& input,
                 std::uint64_t checksum) {
  TickInput tick;
  tick.tick_id = tick_id;
  tick.input = input;
  tick.checksum = checksum;
  recording.ticks.push_back(tick);
}

ReplayRecording record_input_sequence(const MapData& map, const PlayerBody& start,
                                      std::span<const InputFrame> steps, std::uint64_t seed) {
  ReplayRecording recording;
  recording.header.schema_version = kReplaySchemaVersion;
  recording.header.map_id = map.id;
  recording.header.seed = seed;
  recording.header.start_player = start;

  SimulationSession session;
  if (!session.load(map).ok) {
    return recording;
  }
  session.set_player(start);
  recording.header.dt = session.config().dt;

  for (const InputFrame& frame : steps) {
    session.tick(frame);
    record_tick(recording, session.tick_id(), frame, runtime_checksum(session, seed));
  }
  return recording;
}

ReplayPlayResult play_recording(SimulationSession& session, const ReplayRecording& recording) {
  ReplayPlayResult result;
  result.checksums_match = true;
  const std::uint64_t seed = recording.header.seed;

  for (const TickInput& tick : recording.ticks) {
    session.tick(tick.input);
    const std::uint64_t actual = runtime_checksum(session, seed);
    result.checksum = actual;
    const bool tick_id_mismatch = tick.tick_id != 0 && tick.tick_id != session.tick_id();
    const bool checksum_mismatch = tick.checksum != 0 && tick.checksum != actual;
    if ((tick_id_mismatch || checksum_mismatch) && !result.first_diverging_tick.has_value()) {
      result.first_diverging_tick = session.tick_id();
      result.checksums_match = false;
    }
  }

  result.player = session.player();
  result.state = session.state();
  if (recording.ticks.empty()) {
    result.checksum = runtime_checksum(session, seed);
  }
  return result;
}

ReplayPlayResult replay_input_sequence(const MapData& map, const ReplayRecording& recording) {
  SimulationSession session;
  if (!session.load(map).ok) {
    return ReplayPlayResult{};
  }
  session.set_player(recording.header.start_player);
  return play_recording(session, recording);
}

bool write_replay(std::string_view path, const ReplayRecording& recording) {
  return write_replay(path, recording, os_files());
}

bool write_replay(std::string_view path, const ReplayRecording& recording, FileStore& files) {
  const std::string json_text = dump_recording(recording).dump(2) + '\n';
  return files.write(path, json_text).ok;
}

std::optional<ReplayRecording> read_replay(std::string_view path) {
  return read_replay(path, os_files());
}

std::optional<ReplayRecording> read_replay(std::string_view path, const FileStore& files) {
  const FileReadResult read = files.read(path);
  if (!read.ok) {
    return std::nullopt;
  }
  try {
    const json root = json::parse(read.bytes.as_text());
    return load_recording(root);
  } catch (...) {
    return std::nullopt;
  }
}

}  // namespace rat
