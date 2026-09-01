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
  recording.header.dt = rat::kSimulationFixedDt;
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
  REQUIRE(rat::write_replay(path.string(), recording));

  const auto read = rat::read_replay(path.string());
  REQUIRE(read.has_value());
  CHECK(read->header.schema_version == rat::kReplaySchemaVersion);
  CHECK(read->header.map_id == "grey_yard");
  CHECK(read->header.seed == 42);
  CHECK(read->header.dt == Approx(rat::kSimulationFixedDt).margin(1e-7f));
  CHECK(read->header.start_player.x == Approx(1.5f).margin(1e-5f));
  CHECK(read->header.start_player.speed == Approx(5.0f).margin(1e-5f));
  REQUIRE(read->ticks.size() == 1);
  CHECK(read->ticks[0].tick_id == 1);
  CHECK(read->ticks[0].checksum == 0xC0FFEEULL);
  CHECK(read->ticks[0].input.jump_pressed);
  CHECK_FALSE(read->ticks[0].input.interact_pressed);
  CHECK(read->ticks[0].input.move.axis_z == Approx(tick.input.move.axis_z).margin(1e-5f));

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
  recording.header.dt = session.config().dt;
  recording.header.start_player = start;

  rat::InputFrame frame;
  frame.move = rat::world_aligned_move(0.0f, 1.0f);
  session.tick(frame);
  rat::record_tick(recording, session.tick_id(), frame,
                   rat::runtime_checksum(session, recording.header.seed));

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

  const rat::ReplayRecording recorded =
      rat::record_input_sequence(map, start, steps, /*seed=*/99);
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

  rat::ReplayRecording recorded = rat::record_input_sequence(map, start, steps, /*seed=*/1);
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
