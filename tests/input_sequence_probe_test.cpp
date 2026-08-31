#include <rat/debug_snapshot.hpp>
#include <rat/input.hpp>
#include <rat/input_sequence.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

using Catch::Approx;

TEST_CASE("Headless input sequence on grey_yard is deterministic", "[probe][mechanics]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);

  rat::SurfaceQuery query(loaded.map);
  rat::PlayerBody start;
  start.x = 1.5f;
  start.z = 1.5f;
  start.y = query.sample(start.x, start.z).y;
  start.speed = 5.0f;

  std::vector<rat::InputFrame> steps;
  steps.push_back({});
  rat::InputFrame interact;
  interact.interact_pressed = true;
  steps.push_back(interact);
  // yard_intro: show_text → variable 0=1 → wait 1 (two blocked ticks after ack).
  steps.push_back({});
  steps.push_back({});

  rat::InputFrame move;
  move.move = rat::world_aligned_move(0.0f, 1.0f);
  constexpr int kMoveFrames = 12;
  for (int i = 0; i < kMoveFrames; ++i) {
    steps.push_back(move);
  }

  const rat::InputSequenceResult result = rat::run_input_sequence(loaded.map, start, steps);

  REQUIRE(result.player.x == Approx(1.5f).margin(1e-4f));
  REQUIRE(result.player.z == Approx(1.0f).margin(1e-4f));
  REQUIRE(result.player.y == Approx(start.y).margin(1e-4f));
  REQUIRE(result.state.player_x() == Approx(result.player.x).margin(1e-4f));
  REQUIRE(result.state.player_z() == Approx(result.player.z).margin(1e-4f));
  CHECK(result.state.map_id() == "grey_yard");
  CHECK(result.state.get_variable(0) == 1);
  CHECK(result.jump.grounded);
  CHECK(result.sim_frame == steps.size());
}

TEST_CASE("Headless input sequence writes a snapshot per step", "[probe]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);

  rat::SurfaceQuery query(loaded.map);
  rat::PlayerBody start;
  start.x = 1.5f;
  start.z = 1.5f;
  start.y = query.sample(start.x, start.z).y;

  const auto dir = std::filesystem::temp_directory_path() / "rat-input-sequence-probe";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);

  rat::InputSequenceConfig config;
  config.write_snapshot_each_step = true;
  config.snapshot_dir = dir.string();

  const std::vector<rat::InputFrame> steps(1);
  const rat::InputSequenceResult result = rat::run_input_sequence(loaded.map, start, steps, config);
  REQUIRE(result.sim_frame == 1);

  const auto path = dir / "step-000001.json";
  REQUIRE(std::filesystem::exists(path));
  const auto snapshot = rat::read_debug_snapshot(path.string());
  REQUIRE(snapshot.has_value());
  CHECK(snapshot->sim_frame == 1);
  CHECK(snapshot->player_x == Approx(start.x).margin(1e-4f));
  CHECK(snapshot->player_z == Approx(start.z).margin(1e-4f));
}
