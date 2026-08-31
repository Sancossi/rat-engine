#include "rat/player.hpp"

#include "rat/map_data.hpp"
#include "rat/surface_query.hpp"

#include <algorithm>
#include <cmath>

namespace rat {
namespace {

float clamp_nonnegative(float value) {
  return std::max(0.0f, value);
}

constexpr float kWalkOffDropEpsilon = 0.05f;
constexpr float kFeetPenetrationEpsilon = 1e-4f;
constexpr float kFeetClearanceEpsilon = 1e-4f;

void launch_jump(JumpState& jump, float jump_speed) {
  jump.grounded = false;
  jump.coyote_time_left = 0.0f;
  jump.jump_buffer_left = 0.0f;
  jump.vertical_speed = std::max(0.0f, jump_speed);
  jump.jump_offset = std::max(0.0f, jump.jump_offset);
}

Aabb2 body_bounds(const PlayerBody& body) {
  return Aabb2{body.x - body.half_extent, body.z - body.half_extent, body.x + body.half_extent,
               body.z + body.half_extent};
}

bool overlaps_blocking(const PlayerBody& body, std::span<const BlockerDef> blockers, float feet_world_y) {
  const Aabb2 body_box = body_bounds(body);
  for (const BlockerDef& blocker : blockers) {
    if (aabb_overlap(body_box, blocker.bounds) &&
        blocker_blocks_feet(blocker, feet_world_y, kFeetClearanceEpsilon)) {
      return true;
    }
  }
  return false;
}

float predict_feet_world_after_step(const JumpState& jump, bool jump_held, float step_dt, float gravity_up,
                                    float gravity_down, float jump_speed, float jump_cut,
                                    float max_fall) {
  if (jump.grounded && jump.jump_offset <= 0.0f && jump.vertical_speed == 0.0f) {
    return jump.jump_offset;
  }

  float vertical_speed = jump.vertical_speed;
  if (!jump_held && vertical_speed > 0.0f) {
    vertical_speed = std::min(vertical_speed, jump_speed * jump_cut);
  }
  const float gravity = vertical_speed < 0.0f ? gravity_down : gravity_up;
  vertical_speed -= gravity * step_dt;
  vertical_speed = std::max(vertical_speed, -max_fall);
  return jump.jump_offset + vertical_speed * step_dt;
}

float depenetration_sign(float axis_delta, float center_axis, float blocker_center_axis) {
  if (std::abs(axis_delta) > 1e-6f) {
    return axis_delta > 0.0f ? 1.0f : -1.0f;
  }
  return center_axis >= blocker_center_axis ? 1.0f : -1.0f;
}

void snap_axis_outside_face(PlayerBody& body, const BlockerDef& blocker, bool axis_x, float sign) {
  if (axis_x) {
    body.x = sign > 0.0f ? blocker.bounds.max_x + body.half_extent + kFeetClearanceEpsilon
                         : blocker.bounds.min_x - body.half_extent - kFeetClearanceEpsilon;
  } else {
    body.z = sign > 0.0f ? blocker.bounds.max_z + body.half_extent + kFeetClearanceEpsilon
                         : blocker.bounds.min_z - body.half_extent - kFeetClearanceEpsilon;
  }
}

bool depenetrate_on_top_crossing_axis(const PlayerBody& from, PlayerBody& to,
                                      std::span<const BlockerDef> blockers, float feet_world_before,
                                      float feet_world_after, bool axis_x) {
  if (feet_world_after >= feet_world_before - kFeetClearanceEpsilon) {
    return false;
  }

  bool changed = false;
  for (const BlockerDef& blocker : blockers) {
    if (!blocker.jumpable || !blocker.top_y.has_value()) {
      continue;
    }
    const float top = *blocker.top_y;
    if (feet_world_before < top - kFeetClearanceEpsilon || feet_world_after >= top - kFeetClearanceEpsilon) {
      continue;
    }

    const float denom = feet_world_before - feet_world_after;
    if (denom <= kFeetClearanceEpsilon) {
      continue;
    }
    const float t = std::clamp((feet_world_before - top) / denom, 0.0f, 1.0f);
    PlayerBody probe = from;
    probe.x = from.x + (to.x - from.x) * t;
    probe.z = from.z + (to.z - from.z) * t;
    if (aabb_overlap(body_bounds(probe), blocker.bounds)) {
      const float axis_delta = axis_x ? (to.x - from.x) : (to.z - from.z);
      const float center_axis = axis_x ? probe.x : probe.z;
      const float blocker_center_axis = axis_x ? (blocker.bounds.min_x + blocker.bounds.max_x) * 0.5f
                                               : (blocker.bounds.min_z + blocker.bounds.max_z) * 0.5f;
      const float sign = depenetration_sign(axis_delta, center_axis, blocker_center_axis);
      snap_axis_outside_face(to, blocker, axis_x, sign);
      changed = true;
    }
  }
  return changed;
}

void depenetrate_grounded_overlap(PlayerBody& body, std::span<const BlockerDef> blockers, float feet_world_y,
                                  float move_axis_x, float move_axis_z) {
  if (!overlaps_blocking(body, blockers, feet_world_y)) {
    return;
  }

  const bool prefer_x = std::abs(move_axis_x) >= std::abs(move_axis_z);
  const auto resolve_axis = [&](bool axis_x, float axis_dir) {
    for (const BlockerDef& blocker : blockers) {
      if (!blocker_blocks_feet(blocker, feet_world_y, kFeetClearanceEpsilon)) {
        continue;
      }
      if (!aabb_overlap(body_bounds(body), blocker.bounds)) {
        continue;
      }
      const float center_axis = axis_x ? body.x : body.z;
      const float blocker_center_axis =
          axis_x ? (blocker.bounds.min_x + blocker.bounds.max_x) * 0.5f
                 : (blocker.bounds.min_z + blocker.bounds.max_z) * 0.5f;
      const float sign = depenetration_sign(axis_dir, center_axis, blocker_center_axis);
      snap_axis_outside_face(body, blocker, axis_x, sign);
    }
  };

  if (prefer_x) {
    resolve_axis(true, move_axis_x);
    if (overlaps_blocking(body, blockers, feet_world_y)) {
      resolve_axis(false, move_axis_z);
    }
  } else {
    resolve_axis(false, move_axis_z);
    if (overlaps_blocking(body, blockers, feet_world_y)) {
      resolve_axis(true, move_axis_x);
    }
  }
}

}  // namespace

JumpState make_grounded_jump_state() {
  JumpState state;
  state.jump_offset = 0.0f;
  state.vertical_speed = 0.0f;
  state.coyote_time_left = 0.0f;
  state.jump_buffer_left = 0.0f;
  state.grounded = true;
  return state;
}

PlayerFrameResult integrate_player_frame_surface(PlayerBody player, JumpState jump,
                                                 const PlayerFrameInput& input, float dt,
                                                 std::span<const BlockerDef> blockers,
                                                 const SurfaceQuery& surface_query,
                                                 const JumpTuning& tuning, float max_step_up) {
  const float safe_dt = std::max(0.0f, dt);
  const float max_substep = std::max(1e-4f, tuning.max_substep_seconds);
  const int substeps =
      std::max(1, static_cast<int>(std::ceil(safe_dt / max_substep)));
  const float step_dt = substeps > 0 ? safe_dt / static_cast<float>(substeps) : 0.0f;

  const float gravity_up = clamp_nonnegative(tuning.gravity);
  const float gravity_down = clamp_nonnegative(tuning.faster_fall_gravity);
  const float jump_speed = clamp_nonnegative(tuning.jump_speed);
  const float jump_cut = std::clamp(tuning.jump_cut, 0.0f, 1.0f);
  const float max_fall = clamp_nonnegative(tuning.max_fall_speed);
  const float coyote_seconds = clamp_nonnegative(tuning.coyote_seconds);
  const float input_buffer_seconds = clamp_nonnegative(tuning.input_buffer_seconds);

  for (int i = 0; i < substeps; ++i) {
    const bool jump_pressed_this_substep = input.jump_pressed && i == 0;
    const bool was_grounded = jump.grounded;
    const SurfaceSample sample_before = surface_query.sample(player.x, player.z);
    const float feet_world_before = sample_before.y + clamp_nonnegative(jump.jump_offset);
    player.y = feet_world_before;
    const float predicted_offset_after_step =
        predict_feet_world_after_step(jump, input.jump_held, step_dt, gravity_up, gravity_down, jump_speed,
                                      jump_cut, max_fall);
    const float feet_world_predicted_after =
        sample_before.y + std::max(0.0f, predicted_offset_after_step);

    if (jump.grounded) {
      player =
          integrate_player_surface(player, input.move, step_dt, blockers, surface_query, max_step_up);
    } else {
      float ix = input.move.axis_x;
      float iz = input.move.axis_z;
      const float move_len = std::sqrt(ix * ix + iz * iz);
      if (move_len > 1e-6f) {
        ix /= move_len;
        iz /= move_len;
      } else {
        ix = 0.0f;
        iz = 0.0f;
      }

      if (std::abs(ix) > 1e-6f) {
        const PlayerBody before_x = player;
        const MoveInput move_x{ix > 0.0f ? 1.0f : -1.0f, 0.0f};
        player = integrate_player(player, move_x, step_dt * std::abs(ix), blockers);
        const SurfaceSample candidate_x = surface_query.sample(player.x, player.z);
        if (candidate_x.y > feet_world_before + kFeetPenetrationEpsilon) {
          player = before_x;
        } else {
          depenetrate_on_top_crossing_axis(before_x, player, blockers, feet_world_before,
                                           feet_world_predicted_after, true);
        }
      }

      if (std::abs(iz) > 1e-6f) {
        const PlayerBody before_z = player;
        const MoveInput move_z{0.0f, iz > 0.0f ? 1.0f : -1.0f};
        player = integrate_player(player, move_z, step_dt * std::abs(iz), blockers);
        const SurfaceSample candidate_z = surface_query.sample(player.x, player.z);
        if (candidate_z.y > feet_world_before + kFeetPenetrationEpsilon) {
          player = before_z;
        } else {
          depenetrate_on_top_crossing_axis(before_z, player, blockers, feet_world_before,
                                           feet_world_predicted_after, false);
        }
      }
    }
    const SurfaceSample sample_after = surface_query.sample(player.x, player.z);
    const float ground_y = sample_after.y;

    if (!jump.grounded) {
      jump.jump_offset = std::max(0.0f, feet_world_before - ground_y);
    } else if (was_grounded && jump.jump_offset <= 0.0f &&
               !sample_before.on_ramp && !sample_after.on_ramp &&
               sample_after.y < sample_before.y - kWalkOffDropEpsilon) {
      jump.grounded = false;
      jump.vertical_speed = 0.0f;
      jump.jump_offset = sample_before.y - sample_after.y;
      jump.coyote_time_left = coyote_seconds;
    }

    if (jump_pressed_this_substep) {
      jump.jump_buffer_left = input_buffer_seconds;
    } else {
      jump.jump_buffer_left = std::max(0.0f, jump.jump_buffer_left - step_dt);
    }

    if (jump.grounded) {
      jump.coyote_time_left = coyote_seconds;
    } else {
      jump.coyote_time_left = std::max(0.0f, jump.coyote_time_left - step_dt);
    }

    if (jump.jump_buffer_left > 0.0f && (jump.grounded || jump.coyote_time_left > 0.0f)) {
      launch_jump(jump, jump_speed);
    }

    if (!input.jump_held && jump.vertical_speed > 0.0f) {
      jump.vertical_speed = std::min(jump.vertical_speed, jump_speed * jump_cut);
    }

    if (!jump.grounded || jump.jump_offset > 0.0f || jump.vertical_speed != 0.0f) {
      const float gravity = jump.vertical_speed < 0.0f ? gravity_down : gravity_up;
      jump.vertical_speed -= gravity * step_dt;
      jump.vertical_speed = std::max(jump.vertical_speed, -max_fall);
      jump.jump_offset += jump.vertical_speed * step_dt;

      if (jump.jump_offset <= 0.0f) {
        jump.jump_offset = 0.0f;
        jump.vertical_speed = 0.0f;
        jump.grounded = true;
        jump.coyote_time_left = coyote_seconds;

        if (jump.jump_buffer_left > 0.0f) {
          launch_jump(jump, jump_speed);
        }
      }
    }

    if (jump.grounded) {
      jump.vertical_speed = 0.0f;
      jump.jump_offset = 0.0f;
    }
    player.y = ground_y + std::max(0.0f, jump.jump_offset);

    if (jump.grounded) {
      depenetrate_grounded_overlap(player, blockers, player.y, input.move.axis_x, input.move.axis_z);
      const SurfaceSample corrected = surface_query.sample(player.x, player.z);
      player.y = corrected.y;
    }
  }

  return PlayerFrameResult{player, jump};
}

}  // namespace rat
