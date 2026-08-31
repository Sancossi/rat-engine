#include <rat/map_data.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

using Catch::Approx;

namespace {

rat::MapData make_surface_map(int width, int height, std::vector<float> ground_y) {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "player_jump_surface";
  map.width = width;
  map.height = height;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = width;
  map.height_grid.height = height;
  map.height_grid.ground_y = std::move(ground_y);
  return map;
}

rat::BlockerDef make_full_wall(float min_x, float min_z, float max_x, float max_z) {
  rat::BlockerDef blocker;
  blocker.bounds = rat::Aabb2{min_x, min_z, max_x, max_z};
  blocker.jumpable = false;
  return blocker;
}

rat::BlockerDef make_low_jumpable(float min_x, float min_z, float max_x, float max_z,
                                  float base_y, float top_y) {
  rat::BlockerDef blocker;
  blocker.bounds = rat::Aabb2{min_x, min_z, max_x, max_z};
  blocker.base_y = base_y;
  blocker.top_y = top_y;
  blocker.jumpable = true;
  return blocker;
}

float run_jump_peak(const rat::SurfaceQuery& query, bool held) {
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 28.0f;
  tuning.jump_speed = 7.5f;
  tuning.jump_cut = 0.35f;
  tuning.faster_fall_gravity = 42.0f;
  tuning.max_fall_speed = 32.0f;
  tuning.coyote_seconds = 0.1f;
  tuning.input_buffer_seconds = 0.1f;

  float peak = 0.0f;
  constexpr float dt = 1.0f / 120.0f;
  for (int frame = 0; frame < 400; ++frame) {
    rat::PlayerFrameInput input;
    input.jump_pressed = frame == 0;
    input.jump_held = held ? true : frame == 0;

    const rat::PlayerFrameResult result =
        rat::integrate_player_frame_surface(body, jump, input, dt, {}, query, tuning);
    body = result.body;
    jump = result.jump;
    peak = std::max(peak, jump.jump_offset);

    if (frame > 0 && jump.grounded) {
      break;
    }
  }

  return peak;
}

int run_to_landing_frames(const rat::SurfaceQuery& query, rat::JumpTuning tuning) {
  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  constexpr float dt = 1.0f / 120.0f;
  for (int frame = 0; frame < 600; ++frame) {
    rat::PlayerFrameInput input;
    input.jump_pressed = frame == 0;
    input.jump_held = frame <= 8;
    const rat::PlayerFrameResult result =
        rat::integrate_player_frame_surface(body, jump, input, dt, {}, query, tuning);
    body = result.body;
    jump = result.jump;
    if (frame > 0 && jump.grounded) {
      return frame;
    }
  }
  return 600;
}

}  // namespace

TEST_CASE("Jump launches from ground sample", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {1.25f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 1.25f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;

  const rat::PlayerFrameResult result =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 60.0f, {}, query);
  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.jump.jump_offset > 0.0f);
  REQUIRE(result.body.y > 1.25f);
  REQUIRE(result.body.y == Approx(1.25f + result.jump.jump_offset).margin(1e-5f));
}

TEST_CASE("Held jump reaches higher apex than tap", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);

  const float held_peak = run_jump_peak(query, true);
  const float tap_peak = run_jump_peak(query, false);
  REQUIRE(held_peak > tap_peak + 0.15f);
}

TEST_CASE("Coyote jump works in window and fails after timeout", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump_ok;
  jump_ok.grounded = false;
  jump_ok.jump_offset = 0.15f;
  jump_ok.vertical_speed = -1.0f;
  jump_ok.coyote_time_left = 0.03f;
  rat::PlayerFrameInput press;
  press.jump_pressed = true;
  press.jump_held = true;
  const rat::PlayerFrameResult ok =
      rat::integrate_player_frame_surface(body, jump_ok, press, 1.0f / 120.0f, {}, query);
  REQUIRE(ok.jump.vertical_speed > 0.0f);
  REQUIRE(ok.jump.jump_offset > jump_ok.jump_offset);

  rat::JumpState jump_fail;
  jump_fail.grounded = false;
  jump_fail.jump_offset = 0.15f;
  jump_fail.vertical_speed = -1.0f;
  jump_fail.coyote_time_left = 0.0f;
  const rat::PlayerFrameResult fail =
      rat::integrate_player_frame_surface(body, jump_fail, press, 1.0f / 120.0f, {}, query);
  REQUIRE(fail.jump.vertical_speed < 0.0f);
}

TEST_CASE("Buffered press before landing relaunches", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 0.01f;
  jump.vertical_speed = -2.0f;
  jump.coyote_time_left = 0.0f;

  rat::JumpTuning tuning;
  tuning.input_buffer_seconds = 0.12f;
  tuning.gravity = 24.0f;
  tuning.faster_fall_gravity = 40.0f;
  tuning.max_fall_speed = 30.0f;

  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;

  const rat::PlayerFrameResult result =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 60.0f, {}, query, tuning);
  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.jump.vertical_speed > 0.0f);
  REQUIRE(result.jump.jump_offset > 0.0f);
}

TEST_CASE("Faster fall reduces time to land", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);

  rat::JumpTuning base;
  base.gravity = 24.0f;
  base.jump_speed = 7.0f;
  base.jump_cut = 0.35f;
  base.faster_fall_gravity = 24.0f;
  base.max_fall_speed = 30.0f;
  base.coyote_seconds = 0.1f;
  base.input_buffer_seconds = 0.1f;

  rat::JumpTuning fast = base;
  fast.faster_fall_gravity = 48.0f;

  const int base_frames = run_to_landing_frames(query, base);
  const int fast_frames = run_to_landing_frames(query, fast);
  REQUIRE(fast_frames < base_frames);
}

TEST_CASE("Landing sets grounded with zero offset", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.5f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 0.02f;
  jump.vertical_speed = -4.0f;
  jump.coyote_time_left = 0.0f;

  rat::PlayerFrameInput input;
  const rat::PlayerFrameResult result =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, {}, query);
  REQUIRE(result.jump.grounded);
  REQUIRE(result.jump.jump_offset == Approx(0.0f).margin(1e-6f));
  REQUIRE(result.body.y == Approx(0.5f));
}

TEST_CASE("Body Y always equals ground plus nonnegative offset", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.5f, 1.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.2f;
  body.z = 0.5f;
  body.speed = 2.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 26.0f;
  tuning.jump_speed = 7.0f;
  tuning.jump_cut = 0.4f;
  tuning.faster_fall_gravity = 40.0f;
  tuning.max_fall_speed = 28.0f;
  tuning.coyote_seconds = 0.1f;
  tuning.input_buffer_seconds = 0.1f;

  for (int frame = 0; frame < 120; ++frame) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = frame == 1;
    input.jump_held = frame < 20;
    const rat::PlayerFrameResult result =
        rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, tuning);
    body = result.body;
    jump = result.jump;
    const rat::SurfaceSample sample = query.sample(body.x, body.z);
    REQUIRE(jump.jump_offset >= 0.0f);
    REQUIRE(body.y == Approx(sample.y + jump.jump_offset).margin(1e-4f));
  }
}

TEST_CASE("No double jump while airborne", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  constexpr float dt = 1.0f / 120.0f;

  rat::PlayerFrameInput first_press;
  first_press.jump_pressed = true;
  first_press.jump_held = true;
  rat::PlayerFrameResult result =
      rat::integrate_player_frame_surface(body, jump, first_press, dt, {}, query);
  body = result.body;
  jump = result.jump;
  REQUIRE_FALSE(jump.grounded);

  for (int i = 0; i < 4; ++i) {
    rat::PlayerFrameInput hold;
    hold.jump_held = true;
    result = rat::integrate_player_frame_surface(body, jump, hold, dt, {}, query);
    body = result.body;
    jump = result.jump;
  }

  const float before_second_press = jump.vertical_speed;
  rat::PlayerFrameInput second_press;
  second_press.jump_pressed = true;
  second_press.jump_held = true;
  result = rat::integrate_player_frame_surface(body, jump, second_press, dt, {}, query);
  REQUIRE(result.jump.vertical_speed <= before_second_press + 1e-4f);
}

TEST_CASE("Walk-off ledge becomes airborne and keeps feet Y", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(2, 1, {1.0f, 0.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 3.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 20.0f;
  tuning.faster_fall_gravity = 20.0f;
  tuning.coyote_seconds = 0.1f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};
  const rat::PlayerFrameResult result =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, {}, query, tuning);

  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.jump.coyote_time_left > 0.0f);
  REQUIRE(result.jump.jump_offset > 0.8f);
  REQUIRE(result.body.y == Approx(query.sample(result.body.x, result.body.z).y + result.jump.jump_offset)
                               .margin(1e-5f));
}

TEST_CASE("Walk-off coyote jump passes within window", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(2, 1, {1.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 20.0f;
  tuning.faster_fall_gravity = 20.0f;
  tuning.coyote_seconds = 0.1f;

  rat::PlayerFrameInput move_only;
  move_only.move = rat::MoveInput{1.0f, 0.0f};
  rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, move_only, 1.0f / 30.0f, {}, query, tuning);
  body = frame.body;
  jump = frame.jump;
  REQUIRE_FALSE(jump.grounded);

  rat::PlayerFrameInput press;
  press.jump_pressed = true;
  press.jump_held = true;
  frame = rat::integrate_player_frame_surface(body, jump, press, 1.0f / 120.0f, {}, query, tuning);
  REQUIRE(frame.jump.vertical_speed > 0.0f);
}

TEST_CASE("Walk-off coyote jump fails after expiry", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(2, 1, {1.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 20.0f;
  tuning.faster_fall_gravity = 20.0f;
  tuning.coyote_seconds = 0.05f;

  rat::PlayerFrameInput move_only;
  move_only.move = rat::MoveInput{1.0f, 0.0f};
  rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, move_only, 1.0f / 30.0f, {}, query, tuning);
  body = frame.body;
  jump = frame.jump;
  REQUIRE_FALSE(jump.grounded);

  for (int i = 0; i < 8; ++i) {
    frame = rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, {}, query, tuning);
    body = frame.body;
    jump = frame.jump;
  }

  rat::PlayerFrameInput press;
  press.jump_pressed = true;
  press.jump_held = true;
  frame = rat::integrate_player_frame_surface(body, jump, press, 1.0f / 120.0f, {}, query, tuning);
  REQUIRE(frame.jump.vertical_speed <= 0.0f);
}

TEST_CASE("Ramp descent stays grounded with ground follow", "[unit][player][jump]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 2.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  for (int i = 0; i < 5; ++i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{-1.0f, 0.0f};
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, input, 1.0f / 60.0f, {}, query);
    body = frame.body;
    jump = frame.jump;
    REQUIRE(jump.grounded);
    REQUIRE(jump.jump_offset == Approx(0.0f).margin(1e-6f));
  }
}

TEST_CASE("Airborne traversal can cross up if feet are high enough", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(2, 1, {0.0f, 1.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.3f;
  jump.vertical_speed = -0.5f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};
  rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, {}, query);

  REQUIRE(frame.body.x > 1.0f);
  REQUIRE(frame.jump.jump_offset < 0.5f);
  REQUIRE(frame.body.y == Approx(query.sample(frame.body.x, frame.body.z).y + frame.jump.jump_offset)
                               .margin(1e-5f));

  body = frame.body;
  jump = frame.jump;
  for (int i = 0; i < 80 && !jump.grounded; ++i) {
    frame = rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, {}, query);
    body = frame.body;
    jump = frame.jump;
  }
  REQUIRE(jump.grounded);
  REQUIRE(body.y == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("Airborne traversal cannot enter platform above feet", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(2, 1, {0.0f, 2.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.1f;
  jump.vertical_speed = -0.5f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};
  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, {}, query);
  REQUIRE(frame.body.x < 1.0f);
}

TEST_CASE("Jump press is consumed on first internal substep only", "[unit][player][jump]") {
  const rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 0.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.0f;
  jump.vertical_speed = 0.0f;
  jump.coyote_time_left = 0.0f;

  rat::JumpTuning tuning;
  tuning.input_buffer_seconds = 0.01f;
  tuning.max_substep_seconds = 1.0f / 120.0f;
  tuning.gravity = 0.0f;
  tuning.faster_fall_gravity = 0.0f;

  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 0.05f, {}, query, tuning);
  REQUIRE(frame.jump.jump_buffer_left == Approx(0.0f).margin(1e-6f));
}

TEST_CASE("Walk into low jumpable blocker is blocked", "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.8f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 3.0f;
  body.y = 0.0f;

  for (int i = 0; i < 30; ++i) {
    body = rat::integrate_player_surface(body, rat::MoveInput{1.0f, 0.0f}, 1.0f / 60.0f, blockers,
                                         query);
  }
  REQUIRE(body.x < 1.0f);
}

TEST_CASE("Jump over low blocker passes when feet clear top", "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.55f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  for (int frame_i = 0; frame_i < 120; ++frame_i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = frame_i == 0;
    input.jump_held = frame_i < 20;
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(body.x > 1.2f);
}

TEST_CASE("Insufficient jump height stays blocked by low blocker", "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.2f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  for (int frame_i = 0; frame_i < 120; ++frame_i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = frame_i == 0;
    input.jump_held = frame_i < 20;
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(body.x < 1.0f);
}

TEST_CASE("Non-jumpable wall blocks regardless of feet height", "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_full_wall(1.0f, 0.0f, 2.0f, 1.0f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 3.0f;
  jump.vertical_speed = -0.1f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};
  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, blockers, query);
  REQUIRE(frame.body.x < 1.0f);
}

TEST_CASE("Elevated low blocker uses world-space top_y semantics", "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 1, {1.5f, 1.5f, 1.5f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.5f, 1.0f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 1.5f;
  body.z = 0.5f;
  body.speed = 3.0f;

  for (int i = 0; i < 40; ++i) {
    body = rat::integrate_player_surface(body, rat::MoveInput{1.0f, 0.0f}, 1.0f / 60.0f, blockers,
                                         query);
  }
  REQUIRE(body.x > 2.0f);
}

TEST_CASE("Airborne diagonal movement slides on blocked axis", "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 3, {
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
  });
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, -2.0f, 1.2f, 2.0f, 0.0f, 1.0f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.2f;
  body.speed = 3.0f;

  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 0.3f;
  jump.vertical_speed = -0.01f;

  rat::JumpTuning tuning;
  tuning.gravity = 0.0f;
  tuning.faster_fall_gravity = 0.0f;
  tuning.max_fall_speed = 10.0f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 1.0f};
  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 0.2f, blockers, query, tuning);

  REQUIRE(frame.body.x == Approx(0.6f).margin(0.03f));
  REQUIRE(frame.body.z > 0.45f);
}

TEST_CASE("Airborne blocker check uses ground plus jump offset, not stale body y",
          "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.5f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 5.0f;  // Deliberately stale.
  body.z = 0.5f;
  body.speed = 3.0f;

  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 0.2f;
  jump.vertical_speed = 0.0f;

  rat::JumpTuning tuning;
  tuning.gravity = 0.0f;
  tuning.faster_fall_gravity = 0.0f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};
  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, blockers, query, tuning);

  REQUIRE(frame.body.x < 1.0f);
  REQUIRE(frame.body.y == Approx(0.2f).margin(1e-4f));
}

TEST_CASE("Jump over low blocker clears far face and lands outside",
          "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(5, 1, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.55f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 3.0f;
  body.half_extent = 0.4f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  for (int frame_i = 0; frame_i < 120; ++frame_i) {  // 1 sec default tuning
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = frame_i == 0;
    input.jump_held = frame_i < 20;
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(body.x > 2.4f);
  REQUIRE(jump.grounded);
  const rat::Aabb2 body_box{
      body.x - body.half_extent, body.z - body.half_extent, body.x + body.half_extent, body.z + body.half_extent};
  REQUIRE_FALSE(rat::aabb_overlap(body_box, blockers.front().bounds));

  for (int i = 0; i < 30; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
  }
  REQUIRE(body.x > 3.0f);
}

TEST_CASE("Jumpable blocker acts as vertical slab: below base passes",
          "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 1.0f, 2.0f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.3f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 0.3f;
  jump.vertical_speed = 0.0f;

  rat::JumpTuning tuning;
  tuning.gravity = 0.0f;
  tuning.faster_fall_gravity = 0.0f;

  for (int i = 0; i < 90; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query, tuning);
    body = frame.body;
    jump = frame.jump;
  }
  REQUIRE(body.x > 2.0f);
}

TEST_CASE("Jumpable blocker acts as elevated slab: inside range blocks",
          "[unit][player][jump][blocker]") {
  const rat::MapData map = make_surface_map(3, 1, {1.5f, 1.5f, 1.5f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 1.0f, 2.0f),
  };

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 1.5f;
  body.z = 0.5f;
  body.speed = 3.0f;

  for (int i = 0; i < 40; ++i) {
    body = rat::integrate_player_surface(body, rat::MoveInput{1.0f, 0.0f}, 1.0f / 60.0f, blockers,
                                         query);
  }
  REQUIRE(body.x < 1.0f);
}
