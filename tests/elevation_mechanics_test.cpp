#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using Catch::Approx;

namespace {

void drain_messages(rat::EventRuntime& runtime, rat::GameState& state, rat::PlayerBody& player,
                    int max_steps = 90) {
  for (int i = 0; i < max_steps; ++i) {
    runtime.update(state, player, false, 1.0f / 60.0f);
    if (runtime.active_message().has_value()) {
      runtime.acknowledge_message();
      continue;
    }
    if (!runtime.player_input_blocked()) {
      return;
    }
  }
}

const rat::BlockerDef* find_jumpable_blocker(const rat::MapData& map) {
  for (const rat::BlockerDef& blocker : map.blockers) {
    if (blocker.jumpable && blocker.base_y.has_value() && blocker.top_y.has_value()) {
      return &blocker;
    }
  }
  return nullptr;
}

const rat::EventDef* find_event(const rat::MapData& map, const std::string& id) {
  for (const rat::EventDef& event : map.events) {
    if (event.id == id) {
      return &event;
    }
  }
  return nullptr;
}

bool overlaps(const rat::PlayerBody& body, const rat::Aabb2& box) {
  const rat::Aabb2 player_box{
      body.x - body.half_extent, body.z - body.half_extent, body.x + body.half_extent, body.z + body.half_extent};
  return rat::aabb_overlap(player_box, box);
}

}  // namespace

TEST_CASE("Grey yard elevation slice: ramp, jumpable blocker, elevated action", "[mechanics][elevation]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 4);

  const rat::BlockerDef* low_blocker = find_jumpable_blocker(loaded.map);
  REQUIRE(low_blocker != nullptr);

  const rat::EventDef* elevated_event = find_event(loaded.map, "elevated_after_blocker");
  REQUIRE(elevated_event != nullptr);
  REQUIRE(elevated_event->tile.has_value());

  const float event_x =
      (static_cast<float>(elevated_event->tile->x) + 0.5f) * loaded.map.tile_size;
  const float event_z =
      (static_cast<float>(elevated_event->tile->z) + 0.5f) * loaded.map.tile_size;
  const float action_radius = 0.65f * loaded.map.tile_size;

  rat::SurfaceQuery query(loaded.map);
  rat::PlayerBody player;
  player.x = 7.2f;
  player.z = 8.5f;
  player.y = query.sample(player.x, player.z).y;
  player.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  const rat::PlayerBody player_start = player;

  bool became_airborne = false;
  bool jump_started = false;
  bool landed_after_jump = false;
  bool used_support_after_jump = false;
  bool left_support_after_jump = false;
  bool cleared_directly_after_jump = false;
  bool reached_final_grounded_terrain = false;
  int jump_hold_left = 0;
  std::vector<float> ramp_y_samples;
  std::vector<float> platform_y_after_jump;
  const float step_dt = 1.0f / 120.0f;
  const float max_step_x = player.speed * step_dt * 1.2f;
  float prev_x = player.x;
  int prev_support_index = jump.support_blocker_index;
  for (int frame = 0; frame < 360; ++frame) {
    const float blocker_front = low_blocker->bounds.min_x - player.half_extent - 0.02f;
    const bool at_blocker_takeoff = player.x >= blocker_front;
    const bool around_event = std::abs(player.x - event_x) <= 0.03f &&
                              std::abs(player.z - event_z) <= 0.03f;
    const bool inside_action_radius =
        (player.x - event_x) * (player.x - event_x) + (player.z - event_z) * (player.z - event_z) <=
        action_radius * action_radius;

    rat::PlayerFrameInput input;
    if (!jump_started) {
      input.move = rat::MoveInput{1.0f, 0.0f};
      if (at_blocker_takeoff) {
        input.jump_pressed = true;
        input.jump_held = true;
        jump_started = true;
        jump_hold_left = 17;
      }
    } else if (!reached_final_grounded_terrain) {
      const bool keep_moving_to_exit_support = jump.support_blocker_index >= 0;
      const bool keep_moving_to_reach_event = !around_event && !inside_action_radius;
      if (keep_moving_to_exit_support || keep_moving_to_reach_event) {
        input.move = rat::MoveInput{1.0f, 0.0f};
      }
      if (jump_hold_left > 0) {
        input.jump_held = true;
        --jump_hold_left;
      }
    }

    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        player, jump, input, step_dt, loaded.map.blockers, query);
    player = result.body;
    jump = result.jump;
    const float dx = player.x - prev_x;
    if (prev_support_index >= 0 && jump.support_blocker_index >= 0) {
      REQUIRE(dx >= -1e-5f);
      REQUIRE(dx <= max_step_x);
    }
    prev_x = player.x;
    prev_support_index = jump.support_blocker_index;

    if (!jump.grounded) {
      became_airborne = true;
    }
    if (jump.grounded && player.x >= 8.0f && player.x <= 9.0f) {
      ramp_y_samples.push_back(player.y);
    }
    if (jump_started && became_airborne && jump.grounded) {
      landed_after_jump = true;
      if (jump.support_blocker_index >= 0) {
        used_support_after_jump = true;
      }
      const rat::SurfaceSample sample = query.sample(player.x, player.z);
      if (sample.y > 0.5f) {
        platform_y_after_jump.push_back(player.y);
        REQUIRE(player.z >= 8.0f);
        REQUIRE(player.z < 9.0f);
        REQUIRE(player.x >= 8.0f);
        REQUIRE(player.x < 12.0f);
      }
    }
    if (landed_after_jump && jump.support_blocker_index < 0 &&
        player.x > low_blocker->bounds.max_x + player.half_extent &&
        !overlaps(player, low_blocker->bounds)) {
      if (used_support_after_jump) {
        left_support_after_jump = true;
      } else {
        cleared_directly_after_jump = true;
      }
    }
    if (jump.grounded && jump.support_blocker_index < 0 && !overlaps(player, low_blocker->bounds) &&
        player.x > low_blocker->bounds.max_x + player.half_extent &&
        query.sample(player.x, player.z).y > 0.5f &&
        inside_action_radius) {
      reached_final_grounded_terrain = true;
      break;
    }
  }

  REQUIRE(player.x > player_start.x);
  REQUIRE(std::abs(player.z - player_start.z) < 0.03f);
  REQUIRE(ramp_y_samples.size() >= 3);
  for (std::size_t i = 1; i < ramp_y_samples.size(); ++i) {
    REQUIRE(ramp_y_samples[i] + 1e-3f >= ramp_y_samples[i - 1]);
  }
  REQUIRE(ramp_y_samples.back() - ramp_y_samples.front() > 0.6f);
  REQUIRE(jump_started);
  REQUIRE(became_airborne);
  REQUIRE(landed_after_jump);
  REQUIRE((cleared_directly_after_jump || (used_support_after_jump && left_support_after_jump)));
  REQUIRE(reached_final_grounded_terrain);
  REQUIRE(jump.grounded);
  REQUIRE(jump.support_blocker_index < 0);
  REQUIRE(platform_y_after_jump.size() >= 3);
  REQUIRE(player.y == Approx(1.0f).margin(0.06f));
  REQUIRE(player.x > low_blocker->bounds.max_x + player.half_extent);
  REQUIRE_FALSE(overlaps(player, low_blocker->bounds));
  REQUIRE((player.x - event_x) * (player.x - event_x) + (player.z - event_z) * (player.z - event_z) <=
          action_radius * action_radius);

  rat::EventRuntime runtime;
  rat::GameState state;
  REQUIRE(runtime.load(loaded.map).ok);
  rat::PlayerBody intro_player;
  intro_player.x = 0.5f;
  intro_player.z = 0.5f;
  intro_player.y = query.sample(0.5f, 0.5f).y;
  drain_messages(runtime, state, intro_player);

  REQUIRE(runtime.has_action_prompt(player, state));
  runtime.update(state, player, true, 1.0f / 60.0f);
  drain_messages(runtime, state, player);
  REQUIRE(state.get_switch(40));

  rat::EventRuntime runtime_low;
  rat::GameState state_low;
  REQUIRE(runtime_low.load(loaded.map).ok);
  drain_messages(runtime_low, state_low, intro_player);

  rat::PlayerBody low_player;
  low_player.x = event_x;
  low_player.z = event_z - 0.55f;
  low_player.y = query.sample(low_player.x, low_player.z).y;
  REQUIRE(low_player.y == Approx(0.0f).margin(1e-4f));
  REQUIRE((low_player.x - event_x) * (low_player.x - event_x) +
              (low_player.z - event_z) * (low_player.z - event_z) <=
          action_radius * action_radius);
  REQUIRE_FALSE(runtime_low.has_action_prompt(low_player, state_low));
  runtime_low.update(state_low, low_player, true, 1.0f / 60.0f);
  REQUIRE_FALSE(state_low.get_switch(40));
}

TEST_CASE("grey_yard south crate bypass walks spawn to scrap without going north",
          "[mechanics][quest][map]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  const rat::MapData& map = loaded.map;
  REQUIRE(map.schema_version == 4);

  rat::SurfaceQuery query(map);
  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 5.0f;
  REQUIRE(player.half_extent == Approx(0.4f));
  rat::JumpState jump = rat::make_grounded_jump_state();

  // South around crates: (2,-1) and (3,-2) must stay open. Never use z>=1 (north bypass).
  const struct Waypoint {
    float x;
    float z;
  } kWaypoints[] = {
      {0.5f, -0.5f},
      {2.5f, -0.5f},
      {2.5f, -1.5f},
      {3.5f, -1.5f},
      {5.5f, -1.5f},
      {6.5f, 0.5f},
  };

  std::size_t waypoint = 0;
  constexpr float kDt = 1.0f / 120.0f;
  constexpr float kArrive = 0.22f;
  int stuck_frames = 0;
  float last_x = player.x;
  float last_z = player.z;
  for (int frame = 0; frame < 3600 && waypoint < 6; ++frame) {
    REQUIRE(player.z < 1.0f);
    REQUIRE(player.y < 0.5f);

    const float dx = kWaypoints[waypoint].x - player.x;
    const float dz = kWaypoints[waypoint].z - player.z;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist <= kArrive) {
      ++waypoint;
      stuck_frames = 0;
      continue;
    }

    rat::PlayerFrameInput input;
    input.move.axis_x = dx / dist;
    input.move.axis_z = dz / dist;
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        player, jump, input, kDt, map.blockers, query, {}, 0.35f, {}, &map);
    player = result.body;
    jump = result.jump;

    const float moved = std::hypot(player.x - last_x, player.z - last_z);
    stuck_frames = moved < 1e-4f ? stuck_frames + 1 : 0;
    last_x = player.x;
    last_z = player.z;
    REQUIRE(stuck_frames < 45);
  }

  REQUIRE(waypoint == 6);
  REQUIRE(player.x == Approx(6.5f).margin(0.35f));
  REQUIRE(player.z == Approx(0.5f).margin(0.35f));
  REQUIRE(player.z < 1.0f);
}
