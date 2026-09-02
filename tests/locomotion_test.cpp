#include <rat/locomotion.hpp>
#include <rat/map_data.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

namespace {

rat::MapData make_grey_floor() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "loco_grey_floor";
  map.width = 1;
  map.height = 1;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  return map;
}

}  // namespace

TEST_CASE("Grounded with zero move classifies as Idle", "[unit][loco]") {
  rat::JumpState jump = rat::make_grounded_jump_state();
  const rat::MoveInput move{};

  const rat::LocomotionState state = rat::locomotion_from(jump, move);

  REQUIRE(state == rat::LocomotionState::Idle);
  CHECK(std::string_view(rat::locomotion_state_name(state)) == "Idle");
}

TEST_CASE("Grounded with non-zero axis classifies as Walk", "[unit][loco]") {
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::MoveInput move;
  move.axis_x = 1.0f;
  move.axis_z = 0.0f;

  const rat::LocomotionState state = rat::locomotion_from(jump, move);

  REQUIRE(state == rat::LocomotionState::Walk);
  CHECK(std::string_view(rat::locomotion_state_name(state)) == "Walk");
}

TEST_CASE("Grounded with axis length at threshold stays Idle", "[unit][loco]") {
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::MoveInput move;
  move.axis_x = 1e-4f;
  move.axis_z = 0.0f;

  REQUIRE(rat::locomotion_from(jump, move) == rat::LocomotionState::Idle);

  move.axis_x = 1e-4f + 1e-6f;
  REQUIRE(rat::locomotion_from(jump, move) == rat::LocomotionState::Walk);
}

TEST_CASE("Airborne with positive vertical speed classifies as Jump", "[unit][loco]") {
  rat::JumpState jump;
  jump.grounded = false;
  jump.vertical_speed = 7.5f;
  rat::MoveInput move;
  move.axis_x = 1.0f;

  const rat::LocomotionState state = rat::locomotion_from(jump, move);

  REQUIRE(state == rat::LocomotionState::Jump);
  CHECK(std::string_view(rat::locomotion_state_name(state)) == "Jump");
}

TEST_CASE("Airborne with non-positive vertical speed classifies as Fall", "[unit][loco]") {
  rat::JumpState jump;
  jump.grounded = false;
  jump.vertical_speed = 0.0f;

  REQUIRE(rat::locomotion_from(jump, {}) == rat::LocomotionState::Fall);
  CHECK(std::string_view(rat::locomotion_state_name(rat::LocomotionState::Fall)) == "Fall");

  jump.vertical_speed = -4.0f;
  REQUIRE(rat::locomotion_from(jump, {}) == rat::LocomotionState::Fall);
}

TEST_CASE("Grounded with positive vertical speed is not Jump", "[unit][loco]") {
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.vertical_speed = 7.5f;
  REQUIRE(jump.grounded);
  REQUIRE(jump.vertical_speed > 0.0f);

  REQUIRE(rat::locomotion_from(jump, {}) == rat::LocomotionState::Idle);

  rat::MoveInput move;
  move.axis_x = 1.0f;
  REQUIRE(rat::locomotion_from(jump, move) == rat::LocomotionState::Walk);
}

TEST_CASE("locomotion_state_name matches animation contract strings", "[unit][loco]") {
  CHECK(std::string_view(rat::locomotion_state_name(rat::LocomotionState::Idle)) == "Idle");
  CHECK(std::string_view(rat::locomotion_state_name(rat::LocomotionState::Walk)) == "Walk");
  CHECK(std::string_view(rat::locomotion_state_name(rat::LocomotionState::Jump)) == "Jump");
  CHECK(std::string_view(rat::locomotion_state_name(rat::LocomotionState::Fall)) == "Fall");
  CHECK(std::string_view(rat::locomotion_state_name(rat::LocomotionState::Climb)) == "Climb");
}

TEST_CASE("Climbing flag classifies as Climb before Idle/Walk/Jump", "[unit][loco]") {
  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.climbing = true;
  rat::MoveInput move;
  move.axis_x = 1.0f;
  REQUIRE(rat::locomotion_from(jump, move) == rat::LocomotionState::Climb);
  jump.grounded = false;
  jump.vertical_speed = 7.5f;
  REQUIRE(rat::locomotion_from(jump, {}) == rat::LocomotionState::Climb);
}

TEST_CASE("East ladder integrate classifies as Climb", "[unit][loco]") {
  rat::MapData map = make_grey_floor();
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 0.5f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  input.move.axis_x = 1.0f;
  const rat::PlayerFrameResult climbed =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {},
                                         &map);
  REQUIRE(climbed.jump.climbing);
  REQUIRE(rat::locomotion_from(climbed.jump, input.move) == rat::LocomotionState::Climb);
}

TEST_CASE("Jump while climbing stays Climb with lockout 0", "[unit][loco]") {
  rat::MapData map = make_grey_floor();
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult mounted =
      rat::integrate_player_frame_surface(body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {},
                                         &map);
  REQUIRE(mounted.jump.climbing);
  input.interact_pressed = false;
  input.jump_pressed = true;
  const rat::PlayerFrameResult jumped =
      rat::integrate_player_frame_surface(mounted.body, mounted.jump, input, 1.0f / 120.0f, {},
                                         query, {}, 0.35f, {}, &map);
  REQUIRE(jumped.jump.climbing);
  REQUIRE(jumped.jump.ladder_lockout_left == 0.0f);
  REQUIRE(rat::locomotion_from(jumped.jump, {}) == rat::LocomotionState::Climb);
}

TEST_CASE("integrate_player_frame_surface classify matches locomotion rules", "[unit][loco]") {
  const rat::MapData map = make_grey_floor();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.speed = 5.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();

  const rat::PlayerFrameResult idle =
      rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 60.0f, {}, query);
  REQUIRE(idle.jump.grounded);
  REQUIRE(rat::locomotion_from(idle.jump, {}) == rat::LocomotionState::Idle);

  body = idle.body;
  jump = idle.jump;
  rat::PlayerFrameInput walk_input;
  walk_input.move.axis_x = 1.0f;
  const rat::PlayerFrameResult walk =
      rat::integrate_player_frame_surface(body, jump, walk_input, 1.0f / 60.0f, {}, query);
  REQUIRE(walk.jump.grounded);
  REQUIRE(rat::locomotion_from(walk.jump, walk_input.move) == rat::LocomotionState::Walk);

  body = walk.body;
  jump = walk.jump;
  rat::PlayerFrameInput jump_input;
  jump_input.jump_pressed = true;
  jump_input.jump_held = true;
  const rat::PlayerFrameResult launch =
      rat::integrate_player_frame_surface(body, jump, jump_input, 1.0f / 60.0f, {}, query);
  REQUIRE_FALSE(launch.jump.grounded);
  REQUIRE(launch.jump.vertical_speed > 0.0f);
  REQUIRE(rat::locomotion_from(launch.jump, jump_input.move) == rat::LocomotionState::Jump);

  body = launch.body;
  jump = launch.jump;
  constexpr int kMaxApexFrames = 600;
  int apex_frames = 0;
  for (; apex_frames < kMaxApexFrames && jump.vertical_speed > 0.0f && !jump.grounded;
       ++apex_frames) {
    const rat::PlayerFrameResult rising =
        rat::integrate_player_frame_surface(body, jump, {}, 1.0f / 120.0f, {}, query);
    body = rising.body;
    jump = rising.jump;
  }
  REQUIRE(apex_frames < kMaxApexFrames);
  REQUIRE_FALSE(jump.grounded);
  REQUIRE(jump.vertical_speed <= 0.0f);
  REQUIRE(rat::locomotion_from(jump, {}) == rat::LocomotionState::Fall);
}
