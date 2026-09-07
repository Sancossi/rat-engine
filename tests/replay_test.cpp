#include <rat/debug_snapshot.hpp>
#include <rat/input.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/replay.hpp>
#include <rat/simulation_session.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <system_error>
#include <vector>

using Catch::Approx;

namespace {

#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif

[[nodiscard]] rat::MapData load_grey_yard() {
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  return loaded.map;
}

[[nodiscard]] rat::PlayerBody grey_yard_start(const rat::MapData& map) {
  rat::SurfaceQuery query(map);
  rat::PlayerBody start;
  start.x = 1.5f;
  start.z = 1.5f;
  start.y = query.sample(start.x, start.z).y;
  start.speed = 5.0f;
  return start;
}

// yard_intro autorun (show_text + control_variable) then walk north.
[[nodiscard]] std::vector<rat::InputFrame> grey_yard_move_and_event_steps() {
  std::vector<rat::InputFrame> steps;
  steps.push_back({});
  rat::InputFrame interact;
  interact.interact_pressed = true;
  steps.push_back(interact);
  steps.push_back({});
  steps.push_back({});
  rat::InputFrame move;
  move.move = rat::world_aligned_move(0.0f, 1.0f);
  for (int i = 0; i < 12; ++i) {
    steps.push_back(move);
  }
  return steps;
}

}  // namespace

TEST_CASE("ReplayRecording serializes header, TickInput, and checksum", "[unit][replay]") {
  rat::ReplayRecording recording;
  recording.header.schema_version = rat::kReplaySchemaVersion;
  recording.header.map_id = "grey_yard";
  recording.header.seed = 42;
  recording.header.config.dt = rat::kSimulationFixedDt;
  recording.header.start_player.x = 1.5f;
  recording.header.start_player.y = 0.0f;
  recording.header.start_player.z = 1.5f;
  recording.header.start_player.speed = 5.0f;

  rat::TickInput tick;
  tick.tick_id = 1;
  tick.input.move = rat::world_aligned_move(0.0f, 1.0f);
  tick.input.jump_pressed = true;
  tick.input.interact_pressed = false;
  tick.checksum = 0xC0FFEEULL;
  recording.ticks.push_back(tick);

  const auto path = std::filesystem::temp_directory_path() / "rat-replay-roundtrip.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_replay(path.string(), recording).ok);

  const auto read = rat::read_replay(path.string());
  REQUIRE(read.ok);
  CHECK(read.recording.header.schema_version == rat::kReplaySchemaVersion);
  CHECK(read.recording.header.map_id == "grey_yard");
  CHECK(read.recording.header.seed == 42);
  CHECK(read.recording.header.config.dt == Approx(rat::kSimulationFixedDt).margin(1e-7f));
  CHECK(read.recording.header.start_player.x == Approx(1.5f).margin(1e-5f));
  CHECK(read.recording.header.start_player.speed == Approx(5.0f).margin(1e-5f));
  REQUIRE(read.recording.ticks.size() == 1);
  CHECK(read.recording.ticks[0].tick_id == 1);
  CHECK(read.recording.ticks[0].checksum == 0xC0FFEEULL);
  CHECK(read.recording.ticks[0].input.jump_pressed);
  CHECK_FALSE(read.recording.ticks[0].input.interact_pressed);
  CHECK(read.recording.ticks[0].input.move.axis_z == Approx(tick.input.move.axis_z).margin(1e-5f));

  std::filesystem::remove(path, ec);
}

TEST_CASE("record_tick stores InputFrame with tick_id and runtime checksum", "[unit][replay]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "replay_flat";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);

  rat::PlayerBody start;
  start.x = 1.5f;
  start.y = 0.0f;
  start.z = 1.5f;
  start.speed = 5.0f;

  rat::SimulationSession session;
  REQUIRE(session.load(map).ok);
  session.set_player(start);

  rat::ReplayRecording recording;
  recording.header.map_id = map.id;
  recording.header.seed = 7;
  recording.header.config.dt = session.config().dt;
  recording.header.start_player = start;

  rat::InputFrame frame;
  frame.move = rat::world_aligned_move(0.0f, 1.0f);
  session.tick(frame);
  REQUIRE(rat::record_tick(recording, session.tick_id(), frame,
                   rat::runtime_checksum(session, recording.header.seed)).ok);

  REQUIRE(recording.ticks.size() == 1);
  CHECK(recording.ticks[0].tick_id == 1);
  CHECK(recording.ticks[0].input.move.axis_z == Approx(frame.move.axis_z).margin(1e-5f));
  CHECK(recording.ticks[0].checksum != 0);
  CHECK(recording.ticks[0].checksum == rat::runtime_checksum(session, 7));
}

TEST_CASE("grey_yard replay of load, move, and event command matches checksum twice",
          "[probe][replay][mechanics]") {
  const rat::MapData map = load_grey_yard();
  const rat::PlayerBody start = grey_yard_start(map);
  const std::vector<rat::InputFrame> steps = grey_yard_move_and_event_steps();

  const auto recording_result = rat::record_input_sequence(map, start, steps, /*seed=*/99);
  REQUIRE(recording_result.ok);
  const rat::ReplayRecording recorded = recording_result.recording;
  REQUIRE(recorded.ticks.size() == steps.size());
  REQUIRE(recorded.header.seed == 99);
  REQUIRE(recorded.header.map_id == "grey_yard");

  const rat::ReplayPlayResult first = rat::replay_input_sequence(map, recorded);
  const rat::ReplayPlayResult second = rat::replay_input_sequence(map, recorded);

  CHECK(first.checksums_match);
  CHECK(second.checksums_match);
  CHECK_FALSE(first.first_diverging_tick.has_value());
  CHECK(first.checksum == second.checksum);
  CHECK(first.checksum == recorded.ticks.back().checksum);
  CHECK(first.state.get_variable(0) == 1);
  CHECK(first.player.z == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("mutated replay input reports first diverging tick", "[unit][replay]") {
  const rat::MapData map = load_grey_yard();
  const rat::PlayerBody start = grey_yard_start(map);
  const std::vector<rat::InputFrame> steps = grey_yard_move_and_event_steps();

  const auto recording_result = rat::record_input_sequence(map, start, steps, /*seed=*/1);
  REQUIRE(recording_result.ok);
  rat::ReplayRecording recorded = recording_result.recording;
  REQUIRE(recorded.ticks.size() > 10);

  const std::uint64_t mutated_tick = recorded.ticks[10].tick_id;
  recorded.ticks[10].input.move = {};

  const rat::ReplayPlayResult result = rat::replay_input_sequence(map, recorded);
  REQUIRE(result.first_diverging_tick.has_value());
  CHECK(*result.first_diverging_tick == mutated_tick);
  CHECK_FALSE(result.checksums_match);
}

TEST_CASE("debug snapshot includes tick, input, and checksum", "[unit][debug][replay]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "snap_replay";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);

  rat::SimulationSession session;
  REQUIRE(session.load(map).ok);
  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;
  session.set_player(player);

  rat::InputFrame frame;
  frame.move = rat::world_aligned_move(1.0f, 0.0f);
  frame.jump_held = true;
  session.tick(frame);

  const std::uint64_t checksum = rat::runtime_checksum(session, 3);
  const rat::DebugSnapshot snapshot =
      rat::make_debug_snapshot(session.tick_id(), session.config().app_mode, session.player(),
                               session.jump(), session.events(), session.state(),
                               frame.interact_pressed, {}, frame, checksum);

  CHECK(snapshot.sim_frame == 1);
  CHECK(snapshot.checksum == checksum);
  CHECK(snapshot.input.jump_held);
  CHECK(snapshot.input.move.axis_x == Approx(frame.move.axis_x).margin(1e-5f));

  const auto path = std::filesystem::temp_directory_path() / "rat-debug-replay-fields.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_debug_snapshot(path.string(), snapshot));
  const auto read = rat::read_debug_snapshot(path.string());
  REQUIRE(read.has_value());
  CHECK(read->sim_frame == 1);
  CHECK(read->checksum == checksum);
  CHECK(read->input.jump_held);
  CHECK(read->input.move.axis_x == Approx(frame.move.axis_x).margin(1e-5f));
  std::filesystem::remove(path, ec);
}

namespace {
rat::MapData replay_flat() {
  const auto map = rat::load_map_from_string(R"({"schema_version":1,"id":"replay","width":8,"height":8,"events":[]})");
  REQUIRE(map.ok); return map.map;
}
rat::MapData route_map(const char* steps) {
  std::string text = R"({"schema_version":1,"id":"replay","width":8,"height":8,"events":[{"id":"npc","tile":{"x":3,"z":3},"pages":[{"trigger":"parallel","commands":[{"op":"set_move_route","through":true,"route":)";
  text += steps; text += R"(}]}]}]})";
  auto map = rat::load_map_from_string(text); REQUIRE(map.ok); return map.map;
}
rat::ReplayRecording empty_replay(const rat::MapData& map) {
  auto result = rat::record_input_sequence(map, {}, {}); REQUIRE(result.ok); return result.recording;
}
}

TEST_CASE("Replay uses recorded full config and nondefault dt including uint64 seed", "[unit][replay][contract]") {
  const auto map = replay_flat();
  rat::SimulationConfig config;
  config.dt = 1.0f / 30.0f;
  config.jump_tuning.coyote_seconds = 0.25f;
  config.jump_tuning.jump_cut = 0.7f;
  config.jump_tuning.ladder_bounce_speed = 3.0f;
  config.interact_buffer_seconds = 0.3f;
  config.max_step_up = 0.1f;
  rat::PlayerBody start; start.x = start.z = 2.5f;
  std::vector<rat::InputFrame> steps(3); for (auto& step : steps) step.move.axis_x = 1;
  const auto made = rat::record_input_sequence(map, start, steps, (std::numeric_limits<std::uint64_t>::max)(), config);
  REQUIRE(made.ok);
  rat::MemoryFileStore files;
  REQUIRE(rat::write_replay("replay", made.recording, files).ok);
  const auto read = rat::read_replay("replay", files); REQUIRE(read.ok);
  CHECK(read.recording.header.seed == (std::numeric_limits<std::uint64_t>::max)());
  CHECK(read.recording.header.config.jump_tuning.coyote_seconds == 0.25f);
  CHECK(read.recording.header.config.jump_tuning.ladder_bounce_speed == 3.0f);
  CHECK(read.recording.header.config.interact_buffer_seconds == 0.3f);
  CHECK(read.recording.header.config.max_step_up == 0.1f);
  const auto played = rat::replay_input_sequence(map, read.recording);
  REQUIRE(played.ok); CHECK(played.checksums_match); CHECK(played.ticks_executed == 3);
  CHECK(played.player.x == Approx(3.0f));
}

TEST_CASE("Replay rejects every incompatible header or malformed tick before mutating session", "[unit][replay][contract]") {
  const auto map = replay_flat();
  const std::vector<rat::InputFrame> inputs(2);
  const auto made = rat::record_input_sequence(map, {}, inputs); REQUIRE(made.ok);
  for (int fault = 0; fault < 10; ++fault) {
    auto recording = made.recording;
    switch (fault) {
      case 0: recording.header.schema_version = 1; break;
      case 1: recording.header.runtime_version = 123; break;
      case 2: recording.header.checksum_version = 123; break;
      case 3: recording.header.map_id = "other"; break;
      case 4: recording.header.map_fingerprint ^= 1; break;
      case 5: recording.header.config.dt = 0.01f; break;
      case 6: recording.ticks[1].tick_id = 1; break;
      case 7: recording.ticks[0].tick_id = 0; break;
      case 8: recording.ticks[1].input.move.axis_x = std::numeric_limits<float>::quiet_NaN(); break;
      case 9: recording.header.start_player.speed = -1; break;
    }
    rat::SimulationSession session; REQUIRE(session.load(map).ok);
    const auto before = rat::runtime_state_bytes(session);
    const auto result = rat::play_recording(session, recording);
    INFO(fault); CHECK_FALSE(result.ok); CHECK_FALSE(result.error.empty());
    CHECK(result.ticks_executed == 0); CHECK(session.tick_id() == 0);
    CHECK(rat::runtime_state_bytes(session) == before);
  }
}

TEST_CASE("Replay parser requires all fields and rejects duplicate keys wrong types and old schema", "[unit][replay][contract]") {
  rat::MemoryFileStore files;
  auto recording = empty_replay(replay_flat());
  REQUIRE(rat::record_tick(recording, 1, {}, 0).ok);
  REQUIRE(rat::write_replay("valid", recording, files).ok);
  const auto valid = nlohmann::json::parse(files.read("valid").bytes.as_text());
  auto reject = [&](nlohmann::json value) {
    REQUIRE(files.write("bad", value.dump()).ok);
    const auto result = rat::read_replay("bad", files);
    CHECK_FALSE(result.ok); CHECK_FALSE(result.error.empty());
  };
  reject(nlohmann::json::object());
  for (const char* key : {"map_id", "map_fingerprint", "runtime_version", "checksum_version", "seed", "config", "start_player"}) {
    auto bad = valid; bad["header"].erase(key); reject(bad);
  }
  auto bad = valid; bad["schema_version"] = 1; reject(bad);
  REQUIRE(files.write("legacy", R"({"schema_version":1})").ok);
  CHECK(rat::read_replay("legacy", files).error.find("re-record") != std::string::npos);
  bad = valid; bad["ticks"][0].erase("checksum"); reject(bad);
  bad = valid; bad["ticks"][0]["tick_id"] = -1; reject(bad);
  bad = valid; bad["header"]["seed"] = -1; reject(bad);
  bad = valid; bad["header"]["config"]["dt"] = 1e100; reject(bad);
  bad = valid; bad["ticks"][0]["input"]["jump_pressed"] = 1; reject(bad);
  bad = valid; bad["header"]["config"]["jump_tuning"].erase("gravity"); reject(bad);
  bad = valid; bad["header"]["config"]["jump_tuning"]["max_substep_seconds"] = 0; reject(bad);
  std::string duplicate = "{\"schema_version\":2," + valid.dump().substr(1);
  REQUIRE(files.write("bad", duplicate).ok); CHECK_FALSE(rat::read_replay("bad", files).ok);
  CHECK(rat::read_replay("missing", files).code == rat::ReplayErrorCode::Io);
}

TEST_CASE("Zero recorded checksum is compared and reports divergence", "[unit][replay][contract]") {
  auto recording = empty_replay(replay_flat());
  REQUIRE(rat::record_tick(recording, 1, {}, 0).ok);
  const auto result = rat::replay_input_sequence(replay_flat(), recording);
  REQUIRE(result.ok); CHECK_FALSE(result.checksums_match);
  REQUIRE(result.first_diverging_tick); CHECK(*result.first_diverging_tick == 1);
}

TEST_CASE("Empty replay checks semantic map fingerprint but ignores node layout stale commands and signed zero", "[unit][replay][contract]") {
  auto map = route_map(R"([{"op":"wait","frames":5}])");
  auto recording = empty_replay(map);
  REQUIRE(rat::replay_input_sequence(map, recording).ok);
  auto changed = map; changed.height_grid.ground_y[0] = 2;
  const auto mismatch = rat::replay_input_sequence(changed, recording);
  CHECK_FALSE(mismatch.ok); CHECK(mismatch.ticks_executed == 0);
  auto compiled = rat::compile_map_data(map); REQUIRE(compiled.ok);
  auto presentation = compiled.runtime.data;
  auto& page = presentation.events[0].pages[0];
  page.graph->nodes[0].layout = rat::EventGraphNodeLayout{200, -100};
  rat::Command stale; stale.op = rat::CommandOp::TransferPlayer; stale.map_id = "other";
  page.commands = {stale};
  presentation.height_grid.ground_y[0] = -0.0f;
  const auto same = rat::replay_input_sequence(presentation, recording);
  CHECK(same.ok); CHECK(same.checksums_match);
}

TEST_CASE("Fresh recording rejects tick zero progress pending input and altered jump state", "[unit][replay][contract]") {
  const auto map = replay_flat();
  for (int fault = 0; fault < 5; ++fault) {
    rat::SimulationSession session; REQUIRE(session.load(map).ok);
    if (fault == 0) {
      rat::GameState save; save.set_map_id(map.id); save.set_variable(9, 1);
      REQUIRE(session.apply_loaded_game(save).ok);
    }
    if (fault == 1) session.note_jump_pressed();
    if (fault == 2) session.note_interact_pressed();
    if (fault == 3) session.jump().vertical_speed = 1;
    if (fault == 4) session.state().set_self_switch("off-map NPC", 'D', true);
    CHECK(session.tick_id() == 0);
    const auto before = rat::runtime_state_bytes(session);
    CHECK_FALSE(rat::begin_recording(session).ok);
    const auto played = rat::play_recording(session, empty_replay(map));
    CHECK_FALSE(played.ok); CHECK(played.ticks_executed == 0);
    CHECK(before == rat::runtime_state_bytes(session));
  }
}

TEST_CASE("Runtime checksum includes off-map switches pending edges buffers and complete jump", "[unit][replay][contract]") {
  const auto map = replay_flat();
  for (int difference = 0; difference < 5; ++difference) {
    rat::SimulationSession a, b; REQUIRE(a.load(map).ok); REQUIRE(b.load(map).ok);
    const auto before = rat::runtime_checksum(a, 3);
    switch (difference) {
      case 0: b.state().set_self_switch("off-map", 'A', true); break;
      case 1: b.note_jump_pressed(); break;
      case 2: b.note_interact_pressed(); break;
      case 3: b.jump().climb_into_z = 0.5f; break;
      case 4: b.jump().support_blocker_index = 2; break;
    }
    CHECK(before != rat::runtime_checksum(b, 3));
  }
  rat::SimulationSession a, b; REQUIRE(a.load(map).ok); REQUIRE(b.load(map).ok);
  a.note_interact_pressed(); b.note_interact_pressed();
  a.set_app_mode(rat::AppMode::Edit); a.set_app_mode(rat::AppMode::Play); // drops buffer, pending flag remains
  CHECK(a.input_snapshot().interact_press_pending == b.input_snapshot().interact_press_pending);
  CHECK(a.input_snapshot().interact_seconds_left != b.input_snapshot().interact_seconds_left);
  CHECK(rat::runtime_checksum(a, 0) != rat::runtime_checksum(b, 0));
}

TEST_CASE("Runtime checksum is independent of unordered progress insertion order", "[unit][replay][contract]") {
  rat::SimulationSession a, b; REQUIRE(a.load(replay_flat()).ok); REQUIRE(b.load(replay_flat()).ok);
  for (unsigned id : {1u, 7u, 3u}) { a.state().set_switch(id, true); a.state().set_variable(id, 9); a.state().set_self_switch(std::to_string(id), 'B', true); }
  for (unsigned id : {3u, 7u, 1u}) { b.state().set_switch(id, true); b.state().set_variable(id, 9); b.state().set_self_switch(std::to_string(id), 'B', true); }
  CHECK(rat::runtime_state_bytes(a) == rat::runtime_state_bytes(b));
  CHECK(rat::runtime_checksum(a, 99) == rat::runtime_checksum(b, 99));
}

TEST_CASE("Replay checksum distinguishes moving NPC pose and route index with equal waits", "[unit][replay][contract]") {
  SECTION("opposite motion") {
    rat::SimulationSession a, b;
    REQUIRE(a.load(route_map(R"([{"op":"move","dir":"east"}])")).ok);
    REQUIRE(b.load(route_map(R"([{"op":"move","dir":"west"}])")).ok);
    a.tick({}); b.tick({});
    REQUIRE(a.events().event_overlay("npc")); REQUIRE(b.events().event_overlay("npc"));
    CHECK(a.events().event_overlay("npc")->x != b.events().event_overlay("npc")->x);
    CHECK(rat::runtime_checksum(a, 0) != rat::runtime_checksum(b, 0));
  }
  SECTION("route progress only") {
    rat::SimulationSession a, b;
    REQUIRE(a.load(route_map(R"([{"op":"wait","frames":0},{"op":"wait","frames":5}])")).ok);
    REQUIRE(b.load(route_map(R"([{"op":"wait","frames":6}])")).ok);
    a.tick({}); b.tick({}); a.tick({}); b.tick({});
    const auto x = a.events().replay_snapshot(), y = b.events().replay_snapshot();
    REQUIRE(x.parallels.size() == 1); REQUIRE(y.parallels.size() == 1);
    CHECK(x.parallels[0].wait_frames == y.parallels[0].wait_frames);
    CHECK(x.parallels[0].route_index != y.parallels[0].route_index);
    CHECK(rat::runtime_checksum(a, 0) != rat::runtime_checksum(b, 0));
  }
}

TEST_CASE("Checksum includes last player used for touch entry detection", "[unit][replay][contract]") {
  rat::SimulationSession a, b; REQUIRE(a.load(replay_flat()).ok); REQUIRE(b.load(replay_flat()).ok);
  rat::PlayerBody other; other.x = 3;
  a.events().update(a.state(), a.player(), false, rat::kSimulationFixedDt);
  b.events().update(b.state(), other, false, rat::kSimulationFixedDt);
  CHECK(a.player().x == b.player().x); CHECK(a.tick_id() == b.tick_id());
  CHECK(rat::runtime_checksum(a, 0) != rat::runtime_checksum(b, 0));
}

TEST_CASE("Replay record append and write failures are explicit and transactional", "[unit][replay][contract]") {
  const auto map = replay_flat();
  auto recording = empty_replay(map);
  CHECK_FALSE(rat::record_tick(recording, 2, {}, 0).ok);
  CHECK(recording.ticks.empty());
  rat::InputFrame input; input.climb_move.axis_z = std::numeric_limits<float>::infinity();
  CHECK_FALSE(rat::record_tick(recording, 1, input, 0).ok);
  CHECK(recording.ticks.empty());
  rat::MemoryFileStore files; REQUIRE(files.write("keep", "previous").ok);
  recording.header.checksum_version = 999;
  CHECK_FALSE(rat::write_replay("keep", recording, files).ok);
  CHECK(files.read("keep").bytes.as_text() == "previous");
  auto invalid_map = map; invalid_map.id.clear();
  CHECK_FALSE(rat::record_input_sequence(invalid_map, {}, {}).ok);
  rat::SimulationConfig invalid; invalid.dt = std::numeric_limits<float>::quiet_NaN();
  CHECK_FALSE(rat::record_input_sequence(map, {}, {}, 0, invalid).ok);
}

TEST_CASE("Fingerprint includes graph parameters and does not depend on runtime source revision", "[unit][replay][contract]") {
  const auto map = route_map(R"([{"op":"wait","frames":5}])");
  auto recording = empty_replay(map);
  auto compiled = rat::compile_map_data(map); REQUIRE(compiled.ok);
  compiled.runtime.source_revision = 912;
  rat::SimulationSession session; REQUIRE(session.load(map).ok);
  session.events().load(compiled.runtime);
  CHECK(rat::play_recording(session, recording).ok);
  compiled.runtime.data.events[0].pages[0].graph->nodes[0].route[0].frames = 6;
  CHECK_FALSE(rat::replay_input_sequence(compiled.runtime.data, recording).ok);
}
