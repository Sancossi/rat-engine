#include "rat/replay.hpp"
#include "rat/map_loader.hpp"
#include "strict_json.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace rat {
namespace {
using json = nlohmann::json;
ReplayStatus good() { return {true, ReplayErrorCode::None, {}}; }
ReplayStatus bad(ReplayErrorCode code, std::string error) { return {false, code, std::move(error)}; }

json dump_player(const PlayerBody& v) { return json{{"x", v.x}, {"y", v.y}, {"z", v.z}, {"half_extent", v.half_extent}, {"speed", v.speed}}; }
PlayerBody load_player(const json& node) {
  detail::exact_fields(node, {"x", "y", "z", "half_extent", "speed"});
  PlayerBody v;
  v.x = detail::checked_number<float>(node.at("x"));
  v.y = detail::checked_number<float>(node.at("y"));
  v.z = detail::checked_number<float>(node.at("z"));
  v.half_extent = detail::checked_number<float>(node.at("half_extent"));
  v.speed = detail::checked_number<float>(node.at("speed"));
  return v;
}
json dump_jump(const JumpState& v) { return json{{"jump_offset", v.jump_offset}, {"vertical_speed", v.vertical_speed}, {"coyote_time_left", v.coyote_time_left}, {"jump_buffer_left", v.jump_buffer_left}, {"grounded", v.grounded}, {"support_blocker_index", v.support_blocker_index}, {"ladder_lockout_left", v.ladder_lockout_left}, {"ladder_bounce_x", v.ladder_bounce_x}, {"ladder_bounce_z", v.ladder_bounce_z}, {"climbing", v.climbing}, {"climb_into_x", v.climb_into_x}, {"climb_into_z", v.climb_into_z}}; }
json dump_tuning(const JumpTuning& v) { return json{{"gravity", v.gravity}, {"jump_speed", v.jump_speed}, {"jump_cut", v.jump_cut}, {"faster_fall_gravity", v.faster_fall_gravity}, {"max_fall_speed", v.max_fall_speed}, {"coyote_seconds", v.coyote_seconds}, {"input_buffer_seconds", v.input_buffer_seconds}, {"max_substep_seconds", v.max_substep_seconds}, {"ladder_lockout_seconds", v.ladder_lockout_seconds}, {"ladder_hop_speed", v.ladder_hop_speed}, {"ladder_bounce_speed", v.ladder_bounce_speed}}; }
JumpTuning load_tuning(const json& node) {
  detail::exact_fields(node, {"gravity", "jump_speed", "jump_cut", "faster_fall_gravity", "max_fall_speed", "coyote_seconds", "input_buffer_seconds", "max_substep_seconds", "ladder_lockout_seconds", "ladder_hop_speed", "ladder_bounce_speed"});
  JumpTuning v;
  v.gravity = detail::checked_number<float>(node.at("gravity"));
  v.jump_speed = detail::checked_number<float>(node.at("jump_speed"));
  v.jump_cut = detail::checked_number<float>(node.at("jump_cut"));
  v.faster_fall_gravity = detail::checked_number<float>(node.at("faster_fall_gravity"));
  v.max_fall_speed = detail::checked_number<float>(node.at("max_fall_speed"));
  v.coyote_seconds = detail::checked_number<float>(node.at("coyote_seconds"));
  v.input_buffer_seconds = detail::checked_number<float>(node.at("input_buffer_seconds"));
  v.max_substep_seconds = detail::checked_number<float>(node.at("max_substep_seconds"));
  v.ladder_lockout_seconds = detail::checked_number<float>(node.at("ladder_lockout_seconds"));
  v.ladder_hop_speed = detail::checked_number<float>(node.at("ladder_hop_speed"));
  v.ladder_bounce_speed = detail::checked_number<float>(node.at("ladder_bounce_speed"));
  return v;
}
json dump_interpreter(const InterpreterState& v) { return json{{"event_id", v.event_id}, {"page_index", v.page_index}, {"node_id", v.node_id}, {"wait_frames", v.wait_frames}, {"route_index", v.route_index}, {"route_budget_paid", v.route_budget_paid}, {"waiting_message", v.waiting_message}, {"parallel", v.parallel}, {"autorun", v.autorun}, {"finished", v.finished}}; }

json dump_config(const SimulationConfig& v) {
  return {{"dt", v.dt}, {"app_mode", static_cast<int>(v.app_mode)}, {"jump_tuning", dump_tuning(v.jump_tuning)},
          {"interact_buffer_seconds", v.interact_buffer_seconds}, {"max_step_up", v.max_step_up}};
}
SimulationConfig load_config(const json& node) {
  detail::exact_fields(node, {"dt", "app_mode", "jump_tuning", "interact_buffer_seconds", "max_step_up"});
  SimulationConfig v;
  v.dt = detail::checked_number<float>(node.at("dt"));
  v.app_mode = static_cast<AppMode>(detail::checked_number<int>(node.at("app_mode")));
  v.jump_tuning = load_tuning(node.at("jump_tuning"));
  v.interact_buffer_seconds = detail::checked_number<float>(node.at("interact_buffer_seconds"));
  v.max_step_up = detail::checked_number<float>(node.at("max_step_up"));
  return v;
}
json dump_input(const InputFrame& v) {
  return {{"axis_x", v.move.axis_x}, {"axis_z", v.move.axis_z},
          {"climb_axis_x", v.climb_move.axis_x}, {"climb_axis_z", v.climb_move.axis_z},
          {"jump_pressed", v.jump_pressed},
          {"jump_held", v.jump_held},
          {"interact_pressed", v.interact_pressed},
          {"toggle_mode_pressed", v.toggle_mode_pressed},
          {"hot_apply_pressed", v.hot_apply_pressed},
          {"cycle_camera_pressed", v.cycle_camera_pressed},
          {"debug_snapshot_pressed", v.debug_snapshot_pressed},
          {"undo_pressed", v.undo_pressed},
          {"redo_pressed", v.redo_pressed}};
}
InputFrame load_input(const json& node) {
  detail::exact_fields(node, {"axis_x", "axis_z", "climb_axis_x", "climb_axis_z", "jump_pressed", "jump_held", "interact_pressed", "toggle_mode_pressed", "hot_apply_pressed", "cycle_camera_pressed", "debug_snapshot_pressed", "undo_pressed", "redo_pressed"});
  InputFrame v;
  v.move.axis_x = detail::checked_number<float>(node.at("axis_x"));
  v.move.axis_z = detail::checked_number<float>(node.at("axis_z"));
  v.climb_move.axis_x = detail::checked_number<float>(node.at("climb_axis_x"));
  v.climb_move.axis_z = detail::checked_number<float>(node.at("climb_axis_z"));
  v.jump_pressed = node.at("jump_pressed").get<bool>();
  v.jump_held = node.at("jump_held").get<bool>();
  v.interact_pressed = node.at("interact_pressed").get<bool>();
  v.toggle_mode_pressed = node.at("toggle_mode_pressed").get<bool>();
  v.hot_apply_pressed = node.at("hot_apply_pressed").get<bool>();
  v.cycle_camera_pressed = node.at("cycle_camera_pressed").get<bool>();
  v.debug_snapshot_pressed = node.at("debug_snapshot_pressed").get<bool>();
  v.undo_pressed = node.at("undo_pressed").get<bool>();
  v.redo_pressed = node.at("redo_pressed").get<bool>();
  return v;
}

void validate_values(const ReplayHeader& header) {
  if (header.map_id.empty()) throw std::runtime_error("map_id is required");
  const auto config = load_config(dump_config(header.config)); // validates finite numbers and enum representation
  const auto player = load_player(dump_player(header.start_player));
  if (config.app_mode != AppMode::Play && config.app_mode != AppMode::Edit)
    throw std::runtime_error("invalid app_mode");
  if (config.dt <= 0 || config.dt > kMaxCatchUpSeconds || config.interact_buffer_seconds < 0 || config.max_step_up < 0)
    throw std::runtime_error("invalid simulation dt or configuration range");
  for (const auto& value : dump_tuning(config.jump_tuning))
    if (value.get<float>() < 0) throw std::runtime_error("negative jump tuning");
  if (config.jump_tuning.max_substep_seconds <= 0 || config.jump_tuning.jump_cut > 1 ||
      config.dt / config.jump_tuning.max_substep_seconds > 4096)
    throw std::runtime_error("invalid jump substeps or jump_cut");
  if (player.half_extent <= 0 || player.speed < 0) throw std::runtime_error("invalid start player size or speed");
}
void validate_input(const InputFrame& input) {
  const auto parsed = load_input(dump_input(input));
  for (float value : {parsed.move.axis_x, parsed.move.axis_z, parsed.climb_move.axis_x, parsed.climb_move.axis_z})
    if (std::abs(value) > 1.00001f) throw std::runtime_error("input axis outside [-1, 1]");
}

// Canonical typed encoding: tags, fixed big-endian 64-bit scalars and lengths.
// Object key order is lexical (json's ordered map); array order is preserved.
void u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
  for (int shift = 56; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(v >> shift));
}
void encode(std::vector<std::uint8_t>& out, const json& value) {
  if (value.is_null()) { out.push_back(0); return; }
  if (value.is_boolean()) { out.push_back(value.get<bool>() ? 2 : 1); return; }
  if (value.is_number_float()) {
    out.push_back(3); const double n = value.get<double>();
    u64(out, std::bit_cast<std::uint64_t>(n == 0.0 ? 0.0 : n)); return;
  }
  if (value.is_number_integer()) {
    // Nonnegative signed/unsigned JSON storage is semantically identical.
    const bool negative = !value.is_number_unsigned() && value.get<std::int64_t>() < 0;
    out.push_back(negative ? 4 : 5);
    u64(out, value.get<std::uint64_t>()); return;
  }
  if (value.is_string()) {
    out.push_back(6); const auto& text = value.get_ref<const std::string&>();
    u64(out, text.size()); out.insert(out.end(), text.begin(), text.end()); return;
  }
  out.push_back(value.is_array() ? 7 : 8); u64(out, value.size());
  if (value.is_array()) { for (const auto& item : value) encode(out, item); }
  else for (const auto& [key, item] : value.items()) { encode(out, key); encode(out, item); }
}
std::vector<std::uint8_t> bytes(const json& value) {
  std::vector<std::uint8_t> out; encode(out, value); return out;
}
std::uint64_t hash_bytes(std::span<const std::uint8_t> data, std::uint64_t seed = 0) {
  std::uint64_t hash = 14695981039346656037ull;
  const auto mix = [&](std::uint8_t byte) { hash ^= byte; hash *= 1099511628211ull; };
  std::vector<std::uint8_t> prefix; u64(prefix, seed);
  for (auto byte : prefix) mix(byte);
  for (auto byte : data) mix(byte);
  return hash;
}
json semantic_map(const MapData& data) {
  auto compiled = compile_map_data(data);
  if (!compiled.ok) throw std::runtime_error(format_map_issues(compiled.issues));
  auto map = std::move(compiled.runtime.data);
  map.schema_version = 5; // all runtime geometry, including v1's migrated heights
  for (auto& event : map.events) for (auto& page : event.pages) {
    page.commands.clear(); // graph is authoritative
    if (page.graph) for (auto& node : page.graph->nodes) node.layout.reset();
  }
  for (auto& asset : map.assets) asset.debug_name.clear();
  const auto encoded = serialize_map_to_string(map);
  if (!encoded.ok) throw std::runtime_error(encoded.error);
  return json::parse(encoded.json_text);
}
json snapshot(const SimulationSession& session) {
  const auto& state = session.state();
  json game{{"map_id", state.map_id()}, {"position", {state.player_x(), state.player_y(), state.player_z()}},
            {"switches", json::array()}, {"variables", json::array()},
            {"self_switches", json::array()}, {"inventory", json::array()}};
  for (auto [id, value] : state.debug_switches()) game["switches"].push_back({id, value});
  for (auto [id, value] : state.debug_variables()) game["variables"].push_back({id, value});
  for (auto [id, bits] : state.self_switches_snapshot()) game["self_switches"].push_back({id, bits});
  for (const auto& item : state.inventory()) game["inventory"].push_back({item.id, item.quantity, item.key_item});
  const auto event = session.events().replay_snapshot();
  json vm{{"foreground", event.foreground ? dump_interpreter(*event.foreground) : json(nullptr)},
          {"parallels", json::array()}, {"message", event.active_message ? json(*event.active_message) : json(nullptr)},
          {"touch_inside", event.touch_inside}, {"parallel_started", event.parallel_started},
          {"autorun_lock", event.autorun_lock}, {"overlays", json::array()},
          {"have_last_player", event.have_last_player},
          {"last_player", {event.last_player_x, event.last_player_y, event.last_player_z}}};
  for (const auto& interp : event.parallels) vm["parallels"].push_back(dump_interpreter(interp));
  for (const auto& [id, overlay] : event.overlays)
    vm["overlays"].push_back({id, overlay.tile.x, overlay.tile.z, static_cast<int>(overlay.facing), overlay.x, overlay.z});
  const auto input = session.input_snapshot();
  return {{"checksum_version", kReplayChecksumVersion}, {"tick_id", session.tick_id()},
          {"config", dump_config(session.config())}, {"player", dump_player(session.player())},
          {"jump", dump_jump(session.jump())}, {"game", std::move(game)}, {"events", std::move(vm)},
          {"pending_input", {input.jump_press_pending, input.interact_press_pending, input.interact_seconds_left}}};
}
ReplayStatus fresh_session(const SimulationSession& session, const ReplayHeader& header) {
  try {
    if (session.events().map().id != header.map_id) return bad(ReplayErrorCode::Incompatible, "map_id mismatch");
    const auto semantic = semantic_map(session.events().map());
    if (hash_bytes(bytes(semantic)) != header.map_fingerprint)
      return bad(ReplayErrorCode::Incompatible, "map fingerprint mismatch");
    if (bytes(dump_config(session.config())) != bytes(dump_config(header.config)))
      return bad(ReplayErrorCode::Incompatible, "simulation config mismatch");
    SimulationSession expected(header.config);
    if (!expected.load(session.events().map()).ok) return bad(ReplayErrorCode::InvalidSession, "invalid map");
    expected.set_player(header.start_player);
    if (runtime_state_bytes(expected) != runtime_state_bytes(session))
      return bad(ReplayErrorCode::InvalidSession, "recording/playback requires a fresh session without saved progress, pending input or VM state");
    return good();
  } catch (const std::exception& ex) { return bad(ReplayErrorCode::InvalidSession, ex.what()); }
}
json dump_recording(const ReplayRecording& recording) {
  const auto& h = recording.header;
  json root{{"schema_version", h.schema_version}, {"header", {{"map_id", h.map_id},
    {"map_fingerprint", h.map_fingerprint}, {"runtime_version", h.runtime_version}, {"checksum_version", h.checksum_version},
    {"seed", h.seed}, {"config", dump_config(h.config)}, {"start_player", dump_player(h.start_player)}}},
    {"ticks", json::array()}};
  for (const auto& tick : recording.ticks)
    root["ticks"].push_back({{"tick_id", tick.tick_id}, {"checksum", tick.checksum}, {"input", dump_input(tick.input)}});
  return root;
}
ReplayRecording load_recording(const json& root) {
  detail::exact_fields(root, {"schema_version", "header", "ticks"});
  ReplayRecording out;
  auto& h = out.header;
  h.schema_version = detail::checked_number<std::uint32_t>(root.at("schema_version"));
  const auto& header = root.at("header");
  detail::exact_fields(header, {"map_id", "map_fingerprint", "runtime_version", "checksum_version", "seed", "config", "start_player"});
  h.map_id = header.at("map_id").get<std::string>();
  h.map_fingerprint = detail::checked_number<std::uint64_t>(header.at("map_fingerprint"));
  h.runtime_version = detail::checked_number<std::uint32_t>(header.at("runtime_version"));
  h.checksum_version = detail::checked_number<std::uint32_t>(header.at("checksum_version"));
  h.seed = detail::checked_number<std::uint64_t>(header.at("seed"));
  h.config = load_config(header.at("config"));
  h.start_player = load_player(header.at("start_player"));
  if (!root.at("ticks").is_array()) throw std::runtime_error("ticks must be an array");
  for (const auto& node : root.at("ticks")) {
    detail::exact_fields(node, {"tick_id", "input", "checksum"});
    out.ticks.push_back({detail::checked_number<std::uint64_t>(node.at("tick_id")), load_input(node.at("input")),
                         detail::checked_number<std::uint64_t>(node.at("checksum"))});
  }
  return out;
}
} // namespace

std::vector<std::uint8_t> runtime_state_bytes(const SimulationSession& session) { return bytes(snapshot(session)); }
std::uint64_t runtime_checksum(const SimulationSession& session, std::uint64_t seed) {
  return hash_bytes(runtime_state_bytes(session), seed);
}
ReplayStatus validate_recording(const ReplayRecording& recording) {
  const auto& h = recording.header;
  if (h.schema_version != kReplaySchemaVersion || h.runtime_version != kReplayRuntimeVersion || h.checksum_version != kReplayChecksumVersion)
    return bad(ReplayErrorCode::Incompatible, "unsupported replay contract; re-record this session");
  try {
    validate_values(h);
    for (std::size_t i = 0; i < recording.ticks.size(); ++i) {
      if (recording.ticks[i].tick_id != i + 1) throw std::runtime_error("tick ids must be sequential starting at 1");
      validate_input(recording.ticks[i].input);
    }
    return good();
  } catch (const std::exception& ex) { return bad(ReplayErrorCode::Format, ex.what()); }
}
ReplayRecordResult begin_recording(const SimulationSession& session, std::uint64_t seed) {
  ReplayRecordResult out;
  try {
    auto& h = out.recording.header;
    h.map_id = session.events().map().id;
    h.map_fingerprint = hash_bytes(bytes(semantic_map(session.events().map())));
    h.seed = seed; h.config = session.config(); h.start_player = session.player();
    static_cast<ReplayStatus&>(out) = validate_recording(out.recording);
    if (out.ok) static_cast<ReplayStatus&>(out) = fresh_session(session, h);
  } catch (const std::exception& ex) { static_cast<ReplayStatus&>(out) = bad(ReplayErrorCode::InvalidSession, ex.what()); }
  return out;
}
ReplayStatus record_tick(ReplayRecording& recording, std::uint64_t tick_id, const InputFrame& input, std::uint64_t checksum) {
  auto status = validate_recording(recording);
  if (!status.ok) return status;
  if (tick_id != recording.ticks.size() + 1) return bad(ReplayErrorCode::Format, "nonsequential tick");
  try { validate_input(input); } catch (const std::exception& ex) { return bad(ReplayErrorCode::Format, ex.what()); }
  recording.ticks.push_back({tick_id, input, checksum});
  return good();
}
ReplayRecordResult record_input_sequence(const MapData& map, const PlayerBody& start,
    std::span<const InputFrame> steps, std::uint64_t seed, SimulationConfig config) {
  SimulationSession session(config);
  ReplayRecordResult out;
  try {
    ReplayHeader header; header.map_id = map.id; header.config = config; header.start_player = start;
    validate_values(header);
    for (const auto& input : steps) validate_input(input);
    const auto loaded = session.load(map);
    if (!loaded.ok) throw std::runtime_error(format_map_issues(loaded.issues));
    session.set_player(start);
    out = begin_recording(session, seed);
    if (!out.ok) return out;
    for (const auto& input : steps) {
      session.tick(input);
      out.recording.ticks.push_back({session.tick_id(), input, runtime_checksum(session, seed)});
    }
  } catch (const std::exception& ex) { static_cast<ReplayStatus&>(out) = bad(ReplayErrorCode::InvalidSession, ex.what()); }
  return out;
}
ReplayPlayResult play_recording(SimulationSession& session, const ReplayRecording& recording) {
  ReplayPlayResult out;
  static_cast<ReplayStatus&>(out) = validate_recording(recording);
  if (!out.ok) return out;
  static_cast<ReplayStatus&>(out) = fresh_session(session, recording.header);
  if (!out.ok) return out;
  out.checksums_match = true;
  out.checksum = runtime_checksum(session, recording.header.seed);
  for (const auto& tick : recording.ticks) {
    session.tick(tick.input);
    ++out.ticks_executed;
    out.checksum = runtime_checksum(session, recording.header.seed);
    if (out.checksum != tick.checksum && !out.first_diverging_tick) {
      out.first_diverging_tick = tick.tick_id;
      out.checksums_match = false;
    }
  }
  out.player = session.player(); out.state = session.state();
  return out;
}
ReplayPlayResult replay_input_sequence(const MapData& map, const ReplayRecording& recording) {
  ReplayPlayResult out;
  static_cast<ReplayStatus&>(out) = validate_recording(recording);
  if (!out.ok) return out;
  SimulationSession session(recording.header.config);
  const auto loaded = session.load(map);
  if (!loaded.ok) { static_cast<ReplayStatus&>(out) = bad(ReplayErrorCode::InvalidSession, format_map_issues(loaded.issues)); return out; }
  session.set_player(recording.header.start_player);
  return play_recording(session, recording);
}
ReplayStatus write_replay(std::string_view path, const ReplayRecording& recording) { return write_replay(path, recording, os_files()); }
ReplayStatus write_replay(std::string_view path, const ReplayRecording& recording, FileStore& files) {
  auto status = validate_recording(recording);
  if (!status.ok) return status;
  try {
    const auto written = files.write_atomic(path, dump_recording(recording).dump(2) + "\n");
    return written.ok ? good() : bad(ReplayErrorCode::Io, written.error);
  } catch (const std::exception& ex) { return bad(ReplayErrorCode::Format, ex.what()); }
}
ReplayRecordResult read_replay(std::string_view path) { return read_replay(path, os_files()); }
ReplayRecordResult read_replay(std::string_view path, const FileStore& files) {
  ReplayRecordResult out;
  const auto read = files.read(path);
  if (!read.ok) { static_cast<ReplayStatus&>(out) = bad(ReplayErrorCode::Io, read.error); return out; }
  try {
    const auto root = detail::parse_unique_json(read.bytes.as_text());
    if (root.contains("schema_version") && detail::checked_number<std::uint32_t>(root.at("schema_version")) != kReplaySchemaVersion) {
      static_cast<ReplayStatus&>(out) = bad(ReplayErrorCode::Incompatible, "unsupported replay schema; re-record this session"); return out;
    }
    out.recording = load_recording(root);
    static_cast<ReplayStatus&>(out) = validate_recording(out.recording);
  } catch (const std::exception& ex) { static_cast<ReplayStatus&>(out) = bad(ReplayErrorCode::Format, ex.what()); }
  return out;
}
} // namespace rat
