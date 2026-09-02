#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/simulation_session.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <span>
#include <string>
#include <utility>
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

#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif

[[nodiscard]] rat::MapData load_grey_yard_map() {
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  return loaded.map;
}

struct LoftFallTrace {
  std::vector<float> y_after_leave;
  bool left_support = false;
  bool snapped_to_ground = false;
  bool landed = false;
  float y_at_leave = 0.0f;
  float x_at_leave = 0.0f;
  float z_at_leave = 0.0f;
  float offset_at_leave = 0.0f;
};

[[nodiscard]] rat::PlayerFrameResult step_loft_frame(rat::PlayerBody& body, rat::JumpState& jump,
                                                     const rat::PlayerFrameInput& input,
                                                     const rat::SurfaceQuery& query,
                                                     const rat::MapData& map, float dt) {
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, dt, map.blockers, query, {}, 0.35f, map.edge_barriers, &map);
  body = result.body;
  jump = result.jump;
  return result;
}

// Walk or jump from a loft pose. After leaving slab support, Y must stay near 2 (not 0)
// and then descend. `jump_off` launches on the first frame (control curve).
LoftFallTrace trace_grey_yard_loft_leave(rat::PlayerBody body, rat::JumpState jump,
                                         rat::PlayerFrameInput input, const rat::MapData& map,
                                         const rat::SurfaceQuery& query, bool jump_off) {
  LoftFallTrace trace;
  constexpr float kDt = 1.0f / 120.0f;
  input.jump_pressed = jump_off;
  input.jump_held = jump_off;
  for (int i = 0; i < 400; ++i) {
    (void)step_loft_frame(body, jump, input, query, map, kDt);
    input.jump_pressed = false;
    if (!jump_off) {
      input.jump_held = false;
    }
    if (!trace.left_support && !jump.grounded) {
      trace.left_support = true;
      trace.y_at_leave = body.y;
      trace.x_at_leave = body.x;
      trace.z_at_leave = body.z;
      trace.offset_at_leave = jump.jump_offset;
      if (body.y < 0.5f) {
        trace.snapped_to_ground = true;
      }
    }
    if (trace.left_support && jump.grounded && body.y < 0.5f && i > 0 &&
        trace.y_after_leave.empty()) {
      // Left and landed in the same first airborne observation — one-frame snap.
      trace.snapped_to_ground = true;
    }
    if (trace.left_support) {
      trace.y_after_leave.push_back(body.y);
      if (jump.grounded && body.y < 0.2f) {
        trace.landed = true;
        break;
      }
      if (trace.y_after_leave.size() >= 2) {
        const float prev = trace.y_after_leave[trace.y_after_leave.size() - 2];
        if (prev > 1.5f && body.y < 0.5f && prev - body.y > 1.0f) {
          trace.snapped_to_ground = true;
        }
      }
    }
  }
  return trace;
}

void require_smooth_loft_fall(const LoftFallTrace& walk, const LoftFallTrace& jump_ctrl) {
  REQUIRE(walk.left_support);
  INFO("leave xz=(" << walk.x_at_leave << "," << walk.z_at_leave << ") y=" << walk.y_at_leave
                    << " offset=" << walk.offset_at_leave << " n=" << walk.y_after_leave.size());
  if (walk.y_after_leave.size() >= 3) {
    INFO("y[0..2]=" << walk.y_after_leave[0] << "," << walk.y_after_leave[1] << ","
                    << walk.y_after_leave[2]);
  }
  REQUIRE_FALSE(walk.snapped_to_ground);
  REQUIRE(walk.y_at_leave == Approx(2.0f).margin(0.2f));
  REQUIRE(walk.y_at_leave > 1.5f);
  REQUIRE(walk.y_after_leave.size() >= 8);

  REQUIRE(jump_ctrl.left_support);
  REQUIRE_FALSE(jump_ctrl.snapped_to_ground);
  REQUIRE(jump_ctrl.y_at_leave == Approx(2.0f).margin(0.35f));
  REQUIRE(jump_ctrl.landed);

  bool descended = false;
  for (std::size_t i = 1; i < walk.y_after_leave.size(); ++i) {
    REQUIRE(walk.y_after_leave[i] > -0.05f);
    if (walk.y_after_leave[i] < walk.y_at_leave - 0.2f) {
      descended = true;
    }
    const float drop = walk.y_after_leave[i - 1] - walk.y_after_leave[i];
    REQUIRE(drop < 0.5f);
  }
  REQUIRE(descended);
  REQUIRE(walk.landed);
  REQUIRE(walk.y_after_leave.back() == Approx(0.0f).margin(0.1f));

  // Walk-off starts at rest; jump-off has takeoff speed. Compare walk Y(t) to the
  // same gravity the jump motor uses (gravity then faster_fall_gravity).
  const rat::JumpTuning tuning;
  constexpr float kDt = 1.0f / 120.0f;
  std::size_t fall_start = 0;
  for (std::size_t i = 1; i < walk.y_after_leave.size(); ++i) {
    if (walk.y_after_leave[i] < walk.y_after_leave[i - 1] - 1e-5f) {
      fall_start = i - 1;
      break;
    }
  }
  float v = 0.0f;
  float y = walk.y_after_leave[fall_start];
  const std::size_t compare_n =
      std::min<std::size_t>(20, walk.y_after_leave.size() - fall_start);
  REQUIRE(compare_n >= 6);
  for (std::size_t k = 0; k < compare_n; ++k) {
    REQUIRE(walk.y_after_leave[fall_start + k] == Approx(y).margin(0.12f));
    if (y <= 0.05f) {
      break;
    }
    const float g = v < 0.0f ? tuning.faster_fall_gravity : tuning.gravity;
    v -= g * kDt;
    v = std::max(v, -tuning.max_fall_speed);
    y += v * kDt;
    if (y < 0.0f) {
      y = 0.0f;
    }
  }
}

void ack_grey_yard_intro(rat::SimulationSession& session) {
  session.tick({});
  if (session.events().active_message().has_value()) {
    rat::InputFrame ack;
    ack.interact_pressed = true;
    session.tick(ack);
    session.tick({});
    session.tick({});
  }
  for (int i = 0; i < 30 && session.events().player_input_blocked(); ++i) {
    session.tick({});
  }
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

std::pair<rat::PlayerBody, rat::JumpState> descend_onto_support(
    const rat::SurfaceQuery& query, std::span<const rat::BlockerDef> blockers, float x, float z,
    float top_y) {
  rat::PlayerBody body;
  body.x = x;
  body.z = z;
  body.y = top_y + 0.25f;
  body.speed = 0.0f;

  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = top_y + 0.25f;
  jump.vertical_speed = -0.2f;

  for (int frame_i = 0; frame_i < 240 && !jump.grounded; ++frame_i) {
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }
  return {body, jump};
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

TEST_CASE("Ramp side walk-off becomes airborne and keeps world Y", "[unit][player][jump]") {
  rat::MapData map = make_surface_map(2, 2, {
      0.0f, 0.0f,
      0.0f, 0.0f,
  });
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 1},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.75f;
  body.z = 1.05f;
  body.speed = 3.0f;
  const rat::SurfaceSample before_sample = query.sample(body.x, body.z);
  const float before_world_y = before_sample.y;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 30.0f;
  tuning.faster_fall_gravity = 30.0f;
  tuning.coyote_seconds = 0.1f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{0.0f, -1.0f};
  const rat::PlayerFrameResult result =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 30.0f, {}, query, tuning);
  const rat::SurfaceSample after_sample = query.sample(result.body.x, result.body.z);

  REQUIRE(before_sample.on_ramp);
  REQUIRE_FALSE(after_sample.on_ramp);
  REQUIRE(after_sample.y < before_sample.y - 0.05f);
  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.jump.coyote_time_left > 0.0f);
  REQUIRE(result.body.y <= before_world_y + 1e-4f);
  REQUIRE(result.body.y > before_world_y - 0.06f);
}

TEST_CASE("Ramp side walk-off with baked walls becomes airborne and keeps world Y",
          "[unit][player][jump][surface]") {
  rat::MapData map = make_surface_map(2, 2, {
      0.0f, 0.0f,
      0.0f, 0.0f,
  });
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 1},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.75f;
  body.z = 1.05f;
  body.speed = 3.0f;
  const rat::SurfaceSample before_sample = query.sample(body.x, body.z);
  const float before_world_y = before_sample.y;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 30.0f;
  tuning.faster_fall_gravity = 30.0f;
  tuning.coyote_seconds = 0.1f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{0.0f, -1.0f};
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 30.0f, {}, query, tuning, 0.35f, {}, &map);
  const rat::SurfaceSample after_sample = query.sample(result.body.x, result.body.z);

  REQUIRE(before_sample.on_ramp);
  REQUIRE_FALSE(after_sample.on_ramp);
  REQUIRE(after_sample.y < before_sample.y - 0.05f);
  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.jump.coyote_time_left > 0.0f);
  REQUIRE(result.body.y <= before_world_y + 1e-4f);
  REQUIRE(result.body.y > before_world_y - 0.06f);
}

TEST_CASE("Ramp high-end walk-off with baked walls becomes airborne",
          "[unit][player][jump][surface]") {
  rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  map.ramps.push_back({
      .tile = rat::TileCoord{1, 0},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 1.95f;
  body.z = 0.5f;
  body.speed = 3.0f;
  const rat::SurfaceSample before_sample = query.sample(body.x, body.z);

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 30.0f;
  tuning.faster_fall_gravity = 30.0f;
  tuning.coyote_seconds = 0.1f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 30.0f, {}, query, tuning, 0.35f, {}, &map);
  const rat::SurfaceSample after_sample = query.sample(result.body.x, result.body.z);

  REQUIRE(before_sample.on_ramp);
  REQUIRE_FALSE(after_sample.on_ramp);
  REQUIRE(after_sample.y < before_sample.y - 0.05f);
  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.body.x >= 2.0f);
}

TEST_CASE("Ramp low-end walk-off from an elevated ramp becomes airborne",
          "[unit][player][jump][surface]") {
  rat::MapData map = make_surface_map(3, 1, {0.0f, 1.0f, 2.0f});
  map.ramps.push_back({
      .tile = rat::TileCoord{1, 0},
      .direction = rat::RampDirection::East,
      .low_y = 1.0f,
      .high_y = 2.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 1.05f;
  body.z = 0.5f;
  body.speed = 3.0f;
  const rat::SurfaceSample before_sample = query.sample(body.x, body.z);

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::JumpTuning tuning;
  tuning.gravity = 30.0f;
  tuning.faster_fall_gravity = 30.0f;
  tuning.coyote_seconds = 0.1f;

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{-1.0f, 0.0f};
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 30.0f, {}, query, tuning, 0.35f, {}, &map);
  const rat::SurfaceSample after_sample = query.sample(result.body.x, result.body.z);

  REQUIRE(before_sample.on_ramp);
  REQUIRE_FALSE(after_sample.on_ramp);
  REQUIRE(after_sample.y < before_sample.y - 0.05f);
  REQUIRE_FALSE(result.jump.grounded);
  REQUIRE(result.body.x < 1.0f);
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

TEST_CASE("Descending onto jumpable blocker lands and stays on top",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.7f),
  };

  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.y = 0.9f;
  body.speed = 2.5f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 0.9f;
  jump.vertical_speed = -0.4f;

  bool landed = false;
  for (int frame_i = 0; frame_i < 180; ++frame_i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{0.5f, 0.0f};
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
    if (jump.grounded) {
      landed = true;
      break;
    }
  }

  REQUIRE(landed);
  REQUIRE(jump.grounded);
  REQUIRE(body.y == Approx(0.7f).margin(1e-3f));
  REQUIRE(jump.support_blocker_index == 0);

  for (int i = 0; i < 20; ++i) {
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }
  REQUIRE(jump.grounded);
  REQUIRE(body.y == Approx(0.7f).margin(1e-3f));
  REQUIRE(jump.support_blocker_index == 0);

  const float before_walk_x = body.x;
  body.speed = 2.5f;
  for (int i = 0; i < 10; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
    REQUIRE(jump.grounded);
    REQUIRE(jump.support_blocker_index == 0);
    REQUIRE(body.y == Approx(0.7f).margin(1e-3f));
  }
  REQUIRE(body.x > before_walk_x + 0.05f);
}

TEST_CASE("Moving +X edge overlap lands then does not stick",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.7f),
  };

  rat::PlayerBody body;
  body.x = 0.62f;  // center outside blocker, AABB edge-overlap while moving +X
  body.z = 0.5f;
  body.y = 1.2f;
  body.half_extent = 0.4f;
  body.speed = 3.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.2f;
  jump.vertical_speed = -0.35f;

  for (int i = 0; i < 260 && !jump.grounded; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(jump.support_blocker_index == 0);
  REQUIRE(body.y == Approx(0.7f).margin(1e-3f));
  REQUIRE(body.x > 0.7f);
  REQUIRE(body.x < 2.0f);

  const float landed_x = body.x;
  const rat::PlayerFrameResult next = rat::integrate_player_frame_surface(
      body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
      blockers, query);
  REQUIRE(next.body.x > landed_x + 1e-4f);
  REQUIRE(next.body.y >= 0.0f);
}

TEST_CASE("Walking off blocker top becomes airborne with coyote and preserved world Y",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(5, 1, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.8f),
  };

  auto [body, jump] = descend_onto_support(query, blockers, 1.5f, 0.5f, 0.8f);
  body.speed = 3.0f;
  REQUIRE(jump.grounded);
  const float pre_leave_y = body.y;
  REQUIRE(pre_leave_y == Approx(0.8f).margin(1e-4f));

  rat::PlayerFrameInput move;
  move.move = rat::MoveInput{1.0f, 0.0f};
  bool left_support = false;
  for (int i = 0; i < 80 && jump.grounded; ++i) {
    rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, move, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
    if (!jump.grounded) {
      left_support = true;
      break;
    }
  }

  REQUIRE(left_support);
  REQUIRE_FALSE(jump.grounded);
  REQUIRE(jump.coyote_time_left > 0.0f);
  REQUIRE(body.y == Approx(pre_leave_y).margin(1e-4f));
  REQUIRE(jump.support_blocker_index < 0);
}

TEST_CASE("Jump from blocker top launches from top world height",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(5, 1, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.75f),
  };

  auto [body, jump] = descend_onto_support(query, blockers, 1.5f, 0.5f, 0.75f);
  REQUIRE(jump.grounded);
  REQUIRE(body.y == Approx(0.75f).margin(1e-3f));

  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, blockers, query);

  REQUIRE_FALSE(frame.jump.grounded);
  REQUIRE(frame.jump.vertical_speed > 0.0f);
  REQUIRE(frame.body.y > 0.75f);
  REQUIRE(frame.jump.support_blocker_index < 0);
}

TEST_CASE("Descending onto overlapping blockers selects highest top",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.6f),
      make_low_jumpable(1.1f, 0.0f, 2.1f, 1.0f, 0.0f, 0.9f),
  };

  rat::PlayerBody body;
  body.x = 1.4f;
  body.z = 0.5f;
  body.y = 1.1f;
  body.speed = 0.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.1f;
  jump.vertical_speed = -0.2f;

  for (int frame_i = 0; frame_i < 200 && !jump.grounded; ++frame_i) {
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.y == Approx(0.9f).margin(1e-3f));
  REQUIRE(jump.support_blocker_index == 1);
}

TEST_CASE("Raised terrain above max step remains jump-landable",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(2, 1, {0.0f, 1.2f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.95f;
  body.z = 0.5f;
  body.speed = 3.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.45f;
  jump.vertical_speed = -0.35f;

  bool landed_high = false;
  for (int frame_i = 0; frame_i < 240; ++frame_i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query);
    body = frame.body;
    jump = frame.jump;
    const float sampled_y = query.sample(body.x, body.z).y;
    if (jump.grounded && sampled_y > 1.1f) {
      landed_high = true;
      break;
    }
  }

  REQUIRE(landed_high);
  REQUIRE(body.y == Approx(1.2f).margin(1e-3f));
}

TEST_CASE("Walk-off suppression is substep-local, not frame-wide",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(5, 1, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.8f),
  };
  auto [body, jump] = descend_onto_support(query, blockers, 1.95f, 0.5f, 0.8f);
  body.speed = 15.0f;
  body.x = 2.35f;  // leave support on first substep
  REQUIRE(jump.grounded);

  rat::JumpTuning tuning;
  tuning.gravity = 36.0f;
  tuning.faster_fall_gravity = 36.0f;
  tuning.max_substep_seconds = 1.0f / 120.0f;
  const float before_y = body.y;
  const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
      body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 30.0f, blockers,
      query, tuning);

  REQUIRE_FALSE(frame.jump.grounded);
  REQUIRE(frame.jump.coyote_time_left > 0.0f);
  REQUIRE(frame.body.y < before_y - 1e-4f);
}

TEST_CASE("Depenetration support loss keeps pre-leave world Y and coyote",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(5, 1, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.8f),
      make_full_wall(1.9f, 0.0f, 2.0f, 1.0f),
  };

  rat::PlayerBody body;
  body.x = 1.98f;
  body.z = 0.5f;
  body.y = 0.8f;
  body.half_extent = 0.4f;
  body.speed = 0.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.support_blocker_index = 0;
  const float before_y = body.y;

  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
  REQUIRE_FALSE(frame.jump.grounded);
  REQUIRE(frame.jump.coyote_time_left > 0.0f);
  REQUIRE(frame.jump.support_blocker_index < 0);
  REQUIRE(frame.body.y == Approx(before_y).margin(1e-4f));
}

TEST_CASE("Stale support index clears without upward teleport",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.0f),
      make_low_jumpable(1.0f, 0.0f, 2.0f, 1.0f, 0.0f, 0.8f),
  };

  rat::PlayerBody body;
  body.x = 1.5f;
  body.z = 0.5f;
  body.y = 0.8f;
  body.speed = 0.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.support_blocker_index = 0;  // stale/reordered pointer to wrong top

  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
  REQUIRE(frame.body.y == Approx(0.8f).margin(1e-4f));
  REQUIRE(frame.jump.support_blocker_index != 0);
}

TEST_CASE("Invalid support blocker missing base transitions safely",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  rat::BlockerDef invalid;
  invalid.bounds = rat::Aabb2{1.0f, 0.0f, 2.0f, 1.0f};
  invalid.jumpable = true;
  invalid.top_y = 0.8f;
  invalid.base_y.reset();
  const std::vector<rat::BlockerDef> blockers{invalid};

  rat::PlayerBody body;
  body.x = 1.5f;
  body.z = 0.5f;
  body.y = 0.8f;
  body.speed = 0.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.support_blocker_index = 0;

  const rat::PlayerFrameResult frame =
      rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
  REQUIRE(frame.jump.support_blocker_index < 0);
  REQUIRE(frame.body.y <= 0.8f + 1e-4f);
}

TEST_CASE("Support movement blocks step-up above max_step_up",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 1.35f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(0.9f, 0.0f, 1.9f, 1.0f, 0.0f, 0.8f),
  };
  auto [body, jump] = descend_onto_support(query, blockers, 1.5f, 0.5f, 0.8f);
  body.speed = 3.0f;
  REQUIRE(jump.grounded);
  for (int i = 0; i < 60; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.x < 2.05f);
  REQUIRE(body.y == Approx(0.8f).margin(1e-3f));
}

TEST_CASE("Support movement transitions onto reachable higher terrain",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 1.05f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(0.9f, 0.0f, 1.9f, 1.0f, 0.0f, 0.8f),
  };
  auto [body, jump] = descend_onto_support(query, blockers, 1.5f, 0.5f, 0.8f);
  body.speed = 3.0f;
  REQUIRE(jump.grounded);

  for (int i = 0; i < 40; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.x > 2.05f);
  REQUIRE(body.x < 2.95f);
  REQUIRE(body.y == Approx(1.05f).margin(1e-3f));
  REQUIRE(jump.support_blocker_index < 0);
}

TEST_CASE("Grounded reset clears support index", "[unit][player][jump][support]") {
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.support_blocker_index = 7;
  jump = rat::make_grounded_jump_state();
  REQUIRE(jump.support_blocker_index < 0);
}

TEST_CASE("Thin jumpable blocker can still support landing",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 1, {0.0f, 0.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.3f, 0.4f, 1.55f, 0.6f, 0.0f, 0.65f),
  };

  rat::PlayerBody body;
  body.x = 1.42f;
  body.z = 0.5f;
  body.y = 1.0f;
  body.half_extent = 0.4f;
  body.speed = 0.0f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.0f;
  jump.vertical_speed = -0.25f;

  for (int i = 0; i < 260 && !jump.grounded; ++i) {
    const rat::PlayerFrameResult frame =
        rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, blockers, query);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(jump.support_blocker_index == 0);
  REQUIRE(body.y == Approx(0.65f).margin(1e-3f));
}

TEST_CASE("Grey-yard descend plus X acquires support on growing overlap",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(20, 20, std::vector<float>(400, 1.0f));
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(10.2f, 8.1f, 10.8f, 8.9f, 1.0f, 1.6f),
  };

  rat::PlayerBody body;
  body.x = 10.02f;
  body.z = 8.5f;
  body.y = 2.0f;
  body.half_extent = 0.4f;
  body.speed = 1.2f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.0f;
  jump.vertical_speed = -0.6f;

  bool acquired = false;
  for (int i = 0; i < 240; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
    if (jump.grounded && jump.support_blocker_index == 0) {
      acquired = true;
      break;
    }
  }

  REQUIRE(acquired);
  REQUIRE(jump.grounded);
  REQUIRE(jump.support_blocker_index == 0);
  REQUIRE(body.y == Approx(1.6f).margin(1e-4f));
}

TEST_CASE("Thin both-axes blocker supports moving landing",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(4, 4, std::vector<float>(16, 0.0f));
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(1.20f, 1.20f, 1.70f, 1.70f, 0.0f, 0.65f),
  };

  rat::PlayerBody body;
  body.x = 0.82f;
  body.z = 0.82f;
  body.y = 1.0f;
  body.half_extent = 0.4f;
  body.speed = 1.2f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.0f;
  jump.vertical_speed = -0.45f;

  bool acquired = false;
  for (int i = 0; i < 260; ++i) {
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 1.0f}}, 1.0f / 120.0f,
        blockers, query);
    body = frame.body;
    jump = frame.jump;
    if (jump.grounded && jump.support_blocker_index == 0) {
      acquired = true;
      break;
    }
  }

  REQUIRE(acquired);
  REQUIRE(jump.grounded);
  REQUIRE(jump.support_blocker_index == 0);
  REQUIRE(body.y == Approx(0.65f).margin(1e-4f));
}

TEST_CASE("Grey-yard narrow support tiny +X moves incrementally and leaves cleanly",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(20, 20, std::vector<float>(400, 1.0f));
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(10.2f, 8.1f, 10.8f, 8.9f, 1.0f, 1.6f),
  };

  rat::PlayerBody body;
  body.x = 10.35f;
  body.z = 8.5f;
  body.y = 1.6f;
  body.half_extent = 0.4f;
  body.speed = 4.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.support_blocker_index = 0;

  const float step_dt = 1.0f / 120.0f;
  const float max_step_x = body.speed * step_dt * 1.2f;
  const rat::Aabb2 blocker_box = blockers.front().bounds;

  const float first_x = body.x;
  rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
      body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, step_dt, blockers, query);
  body = frame.body;
  jump = frame.jump;
  REQUIRE(jump.grounded);
  REQUIRE(jump.support_blocker_index == 0);
  REQUIRE(body.y == Approx(1.6f).margin(1e-4f));
  REQUIRE(body.x > first_x);
  REQUIRE(body.x - first_x <= max_step_x);

  float prev_x = body.x;
  bool left_support = false;
  for (int i = 0; i < 240; ++i) {
    frame = rat::integrate_player_frame_surface(
        body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{1.0f, 0.0f}}, step_dt, blockers, query);
    body = frame.body;
    jump = frame.jump;
    const float dx = body.x - prev_x;
    REQUIRE(dx >= -1e-5f);
    REQUIRE(dx <= max_step_x);
    prev_x = body.x;
    if (!jump.grounded) {
      left_support = true;
      break;
    }
  }
  REQUIRE(left_support);
  REQUIRE(jump.support_blocker_index < 0);
  REQUIRE(jump.coyote_time_left > 0.0f);
  REQUIRE(body.y == Approx(1.6f).margin(1e-4f));

  const rat::Aabb2 body_box{body.x - body.half_extent, body.z - body.half_extent, body.x + body.half_extent,
                            body.z + body.half_extent};
  REQUIRE_FALSE(rat::aabb_overlap(body_box, blocker_box));

  const float leave_x = body.x;
  float prev_y = body.y;
  for (int i = 0; i < 6; ++i) {
    frame = rat::integrate_player_frame_surface(body, jump, {}, step_dt, blockers, query);
    body = frame.body;
    jump = frame.jump;
    REQUIRE(body.x == Approx(leave_x).margin(1e-5f));
    REQUIRE(body.y <= prev_y + 1e-5f);
    prev_y = body.y;
  }
}

TEST_CASE("Grey-yard narrow support tiny +Z moves incrementally",
          "[unit][player][jump][support]") {
  const rat::MapData map = make_surface_map(20, 20, std::vector<float>(400, 1.0f));
  const rat::SurfaceQuery query(map);
  const std::vector<rat::BlockerDef> blockers = {
      make_low_jumpable(10.2f, 8.1f, 10.8f, 8.9f, 1.0f, 1.6f),
  };

  rat::PlayerBody body;
  body.x = 10.5f;
  body.z = 8.25f;
  body.y = 1.6f;
  body.half_extent = 0.4f;
  body.speed = 4.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.support_blocker_index = 0;

  const float step_dt = 1.0f / 120.0f;
  const float max_step_z = body.speed * step_dt * 1.2f;
  const float before_z = body.z;
  const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
      body, jump, rat::PlayerFrameInput{.move = rat::MoveInput{0.0f, 1.0f}}, step_dt, blockers, query);
  REQUIRE(frame.jump.grounded);
  REQUIRE(frame.jump.support_blocker_index == 0);
  REQUIRE(frame.body.y == Approx(1.6f).margin(1e-4f));
  REQUIRE(frame.body.z > before_z);
  REQUIRE(frame.body.z - before_z <= max_step_z);
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

TEST_CASE("Jump hold across a 0.45 east fence reaches the far tile",
          "[unit][player][jump][edge]") {
  rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  map.edge_barriers.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .height = 0.45f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  for (int frame_i = 0; frame_i < 180; ++frame_i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = frame_i == 0;
    input.jump_held = true;
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, map.edge_barriers);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(body.x >= 1.0f);
  REQUIRE(jump.support_blocker_index == -1);
}

TEST_CASE("Jump hold across a 1.6 east fence never reaches the far tile",
          "[unit][player][jump][edge]") {
  rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  map.edge_barriers.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .height = 1.6f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  for (int frame_i = 0; frame_i < 180; ++frame_i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = frame_i == 0;
    input.jump_held = true;
    const rat::PlayerFrameResult frame = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, map.edge_barriers);
    body = frame.body;
    jump = frame.jump;
  }

  REQUIRE(body.x < 1.0f);
  REQUIRE(jump.support_blocker_index == -1);
}

TEST_CASE("Drop from baked cube lands on lower ground and can walk",
          "[unit][player][jump][surface]") {
  const rat::MapData map = make_surface_map(3, 1, {1.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.z = 0.5f;
  body.y = 1.0f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  rat::PlayerFrameInput east;
  east.move = rat::MoveInput{1.0f, 0.0f};

  for (int i = 0; i < 240; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, east, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (jump.grounded && body.x > 1.0f && body.y < 0.1f) {
      break;
    }
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.x > 1.0f);
  REQUIRE(body.y == Approx(0.0f).margin(1e-3f));
  REQUIRE(query.sample(body.x, body.z).y == Approx(0.0f).margin(1e-3f));

  const float x_landed = body.x;
  for (int i = 0; i < 30; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, east, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.x > x_landed + 0.5f);
  REQUIRE(body.y == Approx(0.0f).margin(1e-3f));
}

TEST_CASE("Jump into baked elevation face stops XZ and drops without wedging",
          "[unit][player][jump][surface]") {
  const rat::MapData map = make_surface_map(2, 1, {0.0f, 1.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.55f;
  body.z = 0.5f;
  body.y = 0.0f;
  // Slow enough that the apex cannot fully enter the high cell, so a fall
  // while overlapping the face is the stuck case rather than a vault-on-top.
  body.speed = 1.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  rat::PlayerFrameInput east;
  east.move = rat::MoveInput{1.0f, 0.0f};
  for (int i = 0; i < 30; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, east, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
  }
  REQUIRE(jump.grounded);
  REQUIRE(body.x < 1.0f);
  REQUIRE(body.y == Approx(0.0f).margin(1e-3f));

  for (int i = 0; i < 180; ++i) {
    rat::PlayerFrameInput input;
    input.move = rat::MoveInput{1.0f, 0.0f};
    input.jump_pressed = i == 0;
    input.jump_held = true;
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (i > 0 && jump.grounded) {
      break;
    }
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.y == Approx(0.0f).margin(1e-3f));
  REQUIRE(body.x < 1.0f);
  REQUIRE(query.sample(body.x, body.z).y == Approx(0.0f).margin(1e-3f));

  const float x_landed = body.x;
  rat::PlayerFrameInput west;
  west.move = rat::MoveInput{-1.0f, 0.0f};
  for (int i = 0; i < 30; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, west, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
  }

  REQUIRE(jump.grounded);
  REQUIRE(body.x < x_landed - 0.5f);
  REQUIRE(body.y == Approx(0.0f).margin(1e-3f));
}

TEST_CASE("Jump under a slab does not rise through the underside", "[unit][player][jump]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f; body.y = 0.0f; body.z = 0.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  for (int i = 0; i < 90; ++i) {
    const auto result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    input.jump_pressed = false;
    REQUIRE(body.y + 1.6f <= 1.75f + 1e-3f);
  }
}

TEST_CASE("Jump under a 1.6 slab does not shove feet below support", "[unit][player][jump]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.floor_slabs.push_back({{0, 0}, 1.6f, 0.25f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  constexpr float kYLo = 1.6f - 0.25f;
  constexpr float kDt = 1.0f / 120.0f;
  for (int i = 0; i < 180; ++i) {
    const auto result = rat::integrate_player_frame_surface(
        body, jump, input, kDt, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    input.jump_pressed = false;
    REQUIRE(body.y >= 0.0f);
    if (!jump.grounded) {
      REQUIRE(body.y + 1.6f <= kYLo + 1e-3f);
    }
  }
}

TEST_CASE("Walking off a floor slab stays at world Y then falls", "[unit][player]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 2.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};

  bool left_slab = false;
  for (int i = 0; i < 40; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (body.x >= 1.0f) {
      left_slab = true;
      REQUIRE_FALSE(jump.grounded);
      REQUIRE(body.y == Approx(2.0f).margin(0.15f));
      REQUIRE(body.y > 0.5f);
      break;
    }
  }
  REQUIRE(left_slab);

  const float y_after_leave = body.y;
  bool descended = false;
  bool landed = false;
  for (int i = 0; i < 180; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (!descended && body.y < y_after_leave - 0.2f) {
      descended = true;
    }
    if (jump.grounded && body.y < 0.1f) {
      landed = true;
      break;
    }
  }
  REQUIRE(descended);
  REQUIRE(landed);
  REQUIRE(body.y == Approx(0.0f).margin(0.05f));
}

TEST_CASE("Walking into a slab hole stays at world Y then falls", "[unit][player]") {
  rat::MapData map = make_surface_map(3, 1, {0.0f, 0.0f, 0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.floor_slabs.push_back({{1, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 1.5f;
  body.y = 2.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();

  rat::PlayerFrameInput input;
  input.move = rat::MoveInput{1.0f, 0.0f};

  bool entered_hole = false;
  for (int i = 0; i < 40; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (body.x >= 2.0f) {
      entered_hole = true;
      REQUIRE_FALSE(jump.grounded);
      REQUIRE(body.y == Approx(2.0f).margin(0.15f));
      REQUIRE(body.y > 0.5f);
      break;
    }
  }
  REQUIRE(entered_hole);

  const float y_after_leave = body.y;
  bool descended = false;
  bool landed = false;
  for (int i = 0; i < 180; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (!descended && body.y < y_after_leave - 0.2f) {
      descended = true;
    }
    if (jump.grounded && body.y < 0.1f) {
      landed = true;
      break;
    }
  }
  REQUIRE(descended);
  REQUIRE(landed);
  REQUIRE(body.y == Approx(0.0f).margin(0.05f));
}

TEST_CASE("make_grounded_jump_state zeros ladder lockout and bounce", "[unit][player]") {
  const rat::JumpState jump = rat::make_grounded_jump_state();
  REQUIRE(jump.ladder_lockout_left == 0.0f);
  REQUIRE(jump.ladder_bounce_x == 0.0f);
  REQUIRE(jump.ladder_bounce_z == 0.0f);
  REQUIRE(jump.climb_into_x == 0.0f);
  REQUIRE(jump.climb_into_z == 0.0f);
  REQUIRE_FALSE(jump.climbing);
}

TEST_CASE("Overlap without interact does not climb", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.move.axis_x = 1.0f;
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE_FALSE(result.jump.climbing);
}

TEST_CASE("grey_yard ground walk west does not pass through east ladder", "[unit][player]") {
  const rat::MapData map = load_grey_yard_map();
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 3.5f;
  body.y = 0.0f;
  body.z = 4.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput walk_west;
  walk_west.move.axis_x = -1.0f;
  constexpr float kDt = 1.0f / 120.0f;
  for (int i = 0; i < 180; ++i) {
    (void)step_loft_frame(body, jump, walk_west, query, map, kDt);
    REQUIRE_FALSE(jump.climbing);
  }
  REQUIRE(body.x >= 3.0f);
  REQUIRE(body.y == Approx(0.0f).margin(0.05f));

  rat::PlayerFrameInput interact;
  interact.interact_pressed = true;
  const rat::PlayerFrameResult mounted =
      step_loft_frame(body, jump, interact, query, map, kDt);
  REQUIRE(mounted.jump.climbing);
  REQUIRE(body.x < 3.0f);
  REQUIRE(body.x > 2.7f);

  const float y_mounted = body.y;
  rat::PlayerFrameInput climb_up;
  climb_up.move.axis_x = -1.0f;
  for (int i = 0; i < 24; ++i) {
    (void)step_loft_frame(body, jump, climb_up, query, map, kDt);
  }
  REQUIRE(jump.climbing);
  REQUIRE(body.y > y_mounted);
}

TEST_CASE("Interact on an east ladder mounts the rail", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  REQUIRE(result.jump.climb_into_x == Approx(1.0f));
  REQUIRE(result.body.x == Approx(0.85f).margin(0.2f));
  REQUIRE(result.body.x > 0.7f);
  REQUIRE(result.body.x < 1.0f);
}

TEST_CASE("Interact on east ladder from +X stores into toward the rungs", "[unit][player]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 1.15f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  REQUIRE(result.jump.climb_into_x == Approx(-1.0f));
  REQUIRE(result.jump.climb_into_z == Approx(0.0f));
  const rat::Vec3 mounted{result.body.x, result.body.y, result.body.z};
  const rat::ClimbCameraPose pose =
      rat::climb_camera_pose(mounted, result.jump.climb_into_x, result.jump.climb_into_z);
  REQUIRE(pose.eye.x > result.body.x);
  const rat::MoveInput w = rat::camera_relative_move(0.0f, 1.0f, pose.eye, pose.focus);
  REQUIRE(w.axis_x < -0.5f);
  REQUIRE(w.axis_x * result.jump.climb_into_x + w.axis_z * result.jump.climb_into_z > 0.0f);
}

TEST_CASE("Jump while climbing does not bounce", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  body = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  input.jump_pressed = true;
  result = rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, {},
                                               0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  REQUIRE(result.jump.ladder_lockout_left == 0.0f);
}

TEST_CASE("East climb up with into-face move raises Y", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  body = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  input.move.axis_x = 1.0f;
  for (int i = 0; i < 12; ++i) {
    result = rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, {},
                                                 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
  }
  REQUIRE(result.body.y > 1.0f);
}

TEST_CASE("Top of east ladder plus up steps onto the same-tile slab", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  body = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  input.move.axis_x = 1.0f;
  for (int i = 0; i < 80; ++i) {
    result = rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, {},
                                                 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (!result.jump.climbing) {
      break;
    }
  }
  REQUIRE_FALSE(result.jump.climbing);
  REQUIRE(result.body.y == Approx(2.0f).margin(0.1f));
  REQUIRE(result.body.x < 0.85f);
}

TEST_CASE("Bottom of east ladder plus down leaves climbing", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 0.1f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  body = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  input.move.axis_x = -1.0f;
  for (int i = 0; i < 20; ++i) {
    result = rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, {},
                                                 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    if (!result.jump.climbing) {
      break;
    }
  }
  REQUIRE_FALSE(result.jump.climbing);
}

TEST_CASE("East ladder tangent A/D does not change XZ", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  const float rail_x = result.body.x;
  const float rail_z = result.body.z;
  body = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  input.move.axis_z = 1.0f;
  for (int i = 0; i < 12; ++i) {
    result = rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, {},
                                                 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
  }
  REQUIRE(result.jump.climbing);
  REQUIRE(result.body.x == Approx(rail_x).margin(1e-4f));
  REQUIRE(result.body.z == Approx(rail_z).margin(1e-4f));
}

TEST_CASE("Ground jump not overlapping a ladder still uses jump_speed", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.vertical_speed == Approx(7.5f).margin(0.3f));
  REQUIRE(result.jump.vertical_speed > 5.0f);
  REQUIRE(result.jump.ladder_lockout_left == 0.0f);
}

TEST_CASE("grey_yard walk into loft hole stays at Y then falls like a jump", "[unit][player]") {
  const rat::MapData map = load_grey_yard_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 1.5f;
  body.y = 2.0f;
  body.z = 5.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput walk;
  walk.move = rat::MoveInput{1.0f, 0.0f};

  const LoftFallTrace walk_trace =
      trace_grey_yard_loft_leave(body, jump, walk, map, query, false);
  const LoftFallTrace jump_trace =
      trace_grey_yard_loft_leave(body, rat::make_grounded_jump_state(), walk, map, query, true);
  require_smooth_loft_fall(walk_trace, jump_trace);
}

TEST_CASE("grey_yard walk off loft south edge stays at Y then falls like a jump",
          "[unit][player]") {
  const rat::MapData map = load_grey_yard_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 1.5f;
  body.y = 2.0f;
  body.z = 4.5f;
  body.speed = 5.0f;
  rat::PlayerFrameInput walk;
  walk.move = rat::MoveInput{0.0f, -1.0f};

  const LoftFallTrace walk_trace = trace_grey_yard_loft_leave(
      body, rat::make_grounded_jump_state(), walk, map, query, false);
  const LoftFallTrace jump_trace = trace_grey_yard_loft_leave(
      body, rat::make_grounded_jump_state(), walk, map, query, true);
  require_smooth_loft_fall(walk_trace, jump_trace);
}

TEST_CASE("grey_yard walk off loft east edge stays at Y then falls like a jump", "[unit][player]") {
  const rat::MapData map = load_grey_yard_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 2.0f;
  body.z = 5.5f;
  body.speed = 5.0f;
  rat::PlayerFrameInput walk;
  walk.move = rat::MoveInput{1.0f, 0.0f};

  const LoftFallTrace walk_trace = trace_grey_yard_loft_leave(
      body, rat::make_grounded_jump_state(), walk, map, query, false);
  const LoftFallTrace jump_trace = trace_grey_yard_loft_leave(
      body, rat::make_grounded_jump_state(), walk, map, query, true);
  require_smooth_loft_fall(walk_trace, jump_trace);
}

TEST_CASE("grey_yard climb east ladder then walk into loft hole falls like a jump",
          "[unit][player]") {
  const rat::MapData map = load_grey_yard_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 2.85f;
  body.y = 0.0f;
  body.z = 4.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result =
      step_loft_frame(body, jump, input, query, map, 1.0f / 120.0f);
  REQUIRE(result.jump.climbing);
  input.interact_pressed = false;
  input.move = rat::MoveInput{1.0f, 0.0f};
  bool dismounted = false;
  for (int i = 0; i < 160; ++i) {
    result = step_loft_frame(body, jump, input, query, map, 1.0f / 120.0f);
    if (!result.jump.climbing && body.y >= 1.9f) {
      dismounted = true;
      break;
    }
  }
  REQUIRE(dismounted);
  REQUIRE(body.y == Approx(2.0f).margin(0.15f));
  REQUIRE(jump.grounded);

  // Leave the ladder tile, then walk into the hole at (2, 5).
  input.move = rat::MoveInput{-1.0f, 0.0f};
  for (int i = 0; i < 40 && body.x > 1.6f; ++i) {
    (void)step_loft_frame(body, jump, input, query, map, 1.0f / 120.0f);
  }
  REQUIRE(body.y == Approx(2.0f).margin(0.15f));
  input.move = rat::MoveInput{0.0f, 1.0f};
  for (int i = 0; i < 40 && body.z < 5.45f; ++i) {
    (void)step_loft_frame(body, jump, input, query, map, 1.0f / 120.0f);
  }
  REQUIRE(body.y == Approx(2.0f).margin(0.15f));
  REQUIRE(jump.grounded);

  const rat::PlayerBody loft_pose = body;
  const rat::JumpState loft_jump = jump;
  rat::PlayerFrameInput into_hole;
  into_hole.move = rat::MoveInput{1.0f, 0.0f};
  const LoftFallTrace walk_trace =
      trace_grey_yard_loft_leave(loft_pose, loft_jump, into_hole, map, query, false);
  const LoftFallTrace jump_trace = trace_grey_yard_loft_leave(
      loft_pose, loft_jump, into_hole, map, query, true);
  require_smooth_loft_fall(walk_trace, jump_trace);
}

TEST_CASE("SimulationSession grey_yard walk into loft hole stays at Y then falls",
          "[unit][player][sim]") {
  const rat::MapData map = load_grey_yard_map();
  rat::SimulationSession session;
  REQUIRE(session.load(map).ok);
  ack_grey_yard_intro(session);

  rat::PlayerBody start;
  start.x = 1.5f;
  start.y = 2.0f;
  start.z = 5.5f;
  start.speed = 5.0f;
  session.set_player(start);
  session.jump() = rat::make_grounded_jump_state();

  rat::InputFrame walk;
  walk.move = rat::MoveInput{1.0f, 0.0f};

  bool left_support = false;
  float y_at_leave = 0.0f;
  bool snapped = false;
  bool landed = false;
  std::vector<float> y_after_leave;
  for (int i = 0; i < 400; ++i) {
    session.tick(walk);
    const rat::PlayerBody& body = session.player();
    const rat::JumpState& jump = session.jump();
    if (!left_support && !jump.grounded) {
      left_support = true;
      y_at_leave = body.y;
      if (body.y < 0.5f) {
        snapped = true;
      }
    }
    if (left_support) {
      y_after_leave.push_back(body.y);
      if (y_after_leave.size() >= 2) {
        const float prev = y_after_leave[y_after_leave.size() - 2];
        if (prev > 1.5f && body.y < 0.5f && prev - body.y > 1.0f) {
          snapped = true;
        }
      }
      if (jump.grounded && body.y < 0.2f) {
        landed = true;
        break;
      }
    }
  }

  REQUIRE(left_support);
  REQUIRE_FALSE(snapped);
  REQUIRE(y_at_leave == Approx(2.0f).margin(0.2f));
  REQUIRE(y_after_leave.size() >= 8);
  REQUIRE(landed);
  REQUIRE(session.player().y == Approx(0.0f).margin(0.1f));
}
