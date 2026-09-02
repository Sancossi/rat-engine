#include "rat/player.hpp"

#include "rat/collision.hpp"
#include "rat/locomotion.hpp"
#include "rat/map_data.hpp"
#include "rat/surface_query.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace rat {
namespace {

float clamp_nonnegative(float value) {
  return std::max(0.0f, value);
}

constexpr float kWalkOffDropEpsilon = 0.05f;
constexpr float kFeetPenetrationEpsilon = 1e-4f;
constexpr float kFeetClearanceEpsilon = 1e-4f;
constexpr float kSupportFarFaceMargin = 0.2f;
constexpr float kMinLandingOverlapArea = 0.0f;
constexpr int kInvalidSupportBlockerIndex = -1;

struct SupportCandidate {
  int index = kInvalidSupportBlockerIndex;
  float top_y = 0.0f;
};

bool is_valid_support_blocker(const BlockerDef& blocker) {
  return blocker.jumpable && blocker.base_y.has_value() && blocker.top_y.has_value() &&
         *blocker.top_y >= *blocker.base_y;
}

void launch_jump(JumpState& jump, float jump_speed) {
  jump.grounded = false;
  jump.coyote_time_left = 0.0f;
  jump.jump_buffer_left = 0.0f;
  jump.vertical_speed = std::max(0.0f, jump_speed);
  jump.jump_offset = std::max(0.0f, jump.jump_offset);
  jump.support_blocker_index = kInvalidSupportBlockerIndex;
}

void ladder_face_away(RampDirection face, float& ax, float& az) {
  ax = 0.0f;
  az = 0.0f;
  switch (face) {
    case RampDirection::East:
      ax = -1.0f;
      break;
    case RampDirection::West:
      ax = 1.0f;
      break;
    case RampDirection::North:
      az = 1.0f;
      break;
    case RampDirection::South:
      az = -1.0f;
      break;
  }
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

void try_move_xz(PlayerBody& player, float dx, float dz, std::span<const BlockerDef> blockers,
                 const CollisionWorld& world, float step_up_limit) {
  if (std::abs(dx) <= 1e-8f && std::abs(dz) <= 1e-8f) {
    return;
  }
  const float old_x = player.x;
  const float old_z = player.z;
  player.x += dx;
  player.z += dz;
  if (overlaps_blocking(player, blockers, player.y) ||
      cylinder_hits_walls(collision_body_from_player(player), world, step_up_limit)) {
    player.x = old_x;
    player.z = old_z;
  }
}

bool overlaps_footprint(const PlayerBody& body, const BlockerDef& blocker) {
  return aabb_overlap(body_bounds(body), blocker.bounds);
}

float overlap_depth_1d(float min_a, float max_a, float min_b, float max_b) {
  return std::max(0.0f, std::min(max_a, max_b) - std::max(min_a, min_b));
}

std::optional<SupportCandidate> support_from_index(std::span<const BlockerDef> blockers, int index) {
  if (index < 0 || index >= static_cast<int>(blockers.size())) {
    return std::nullopt;
  }
  const BlockerDef& blocker = blockers[static_cast<std::size_t>(index)];
  if (!is_valid_support_blocker(blocker)) {
    return std::nullopt;
  }
  return SupportCandidate{index, *blocker.top_y};
}

std::optional<SupportCandidate> validate_active_support(const PlayerBody& body,
                                                        std::span<const BlockerDef> blockers,
                                                        int support_index,
                                                        float feet_world_y) {
  const std::optional<SupportCandidate> support = support_from_index(blockers, support_index);
  if (!support.has_value()) {
    return std::nullopt;
  }
  const BlockerDef& blocker = blockers[static_cast<std::size_t>(support->index)];
  if (!overlaps_footprint(body, blocker)) {
    return std::nullopt;
  }
  if (std::abs(feet_world_y - support->top_y) > kFeetClearanceEpsilon) {
    return std::nullopt;
  }
  return support;
}

std::optional<SupportCandidate> resolve_support_for_feet_world(const PlayerBody& body,
                                                               std::span<const BlockerDef> blockers,
                                                               float feet_world_y) {
  std::optional<SupportCandidate> best;
  for (int i = 0; i < static_cast<int>(blockers.size()); ++i) {
    const BlockerDef& blocker = blockers[static_cast<std::size_t>(i)];
    if (!is_valid_support_blocker(blocker) || !overlaps_footprint(body, blocker)) {
      continue;
    }
    const float top = *blocker.top_y;
    if (std::abs(top - feet_world_y) > kFeetClearanceEpsilon) {
      continue;
    }
    if (!best.has_value() || top > best->top_y + kFeetClearanceEpsilon) {
      best = SupportCandidate{i, top};
    }
  }
  return best;
}

std::optional<SupportCandidate> find_descending_support_landing(const PlayerBody& from, const PlayerBody& to,
                                                                std::span<const BlockerDef> blockers,
                                                                float feet_world_before,
                                                                float feet_world_after) {
  if (feet_world_after >= feet_world_before - kFeetClearanceEpsilon) {
    return std::nullopt;
  }
  const float denom = feet_world_before - feet_world_after;
  if (denom <= kFeetClearanceEpsilon) {
    return std::nullopt;
  }

  std::optional<SupportCandidate> best;
  for (int i = 0; i < static_cast<int>(blockers.size()); ++i) {
    const BlockerDef& blocker = blockers[static_cast<std::size_t>(i)];
    if (!is_valid_support_blocker(blocker)) {
      continue;
    }
    const float top = *blocker.top_y;
    if (feet_world_before < top - kFeetClearanceEpsilon || feet_world_after >= top - kFeetClearanceEpsilon) {
      continue;
    }

    const float t = std::clamp((feet_world_before - top) / denom, 0.0f, 1.0f);
    PlayerBody probe = from;
    probe.x = from.x + (to.x - from.x) * t;
    probe.z = from.z + (to.z - from.z) * t;
    const Aabb2 probe_box = body_bounds(probe);
    const Aabb2 final_box = body_bounds(to);
    const bool overlaps_probe = overlaps_footprint(probe, blocker);
    const bool overlaps_final = overlaps_footprint(to, blocker);
    // Require persistent footprint overlap through the substep so that
    // jump-over trajectories that already cleared XZ do not stick to top faces.
    if (!overlaps_probe || !overlaps_final) {
      continue;
    }
    const float move_x = to.x - probe.x;
    const float move_z = to.z - probe.z;
    const bool cleared_far_x =
        (move_x > kFeetClearanceEpsilon &&
         final_box.max_x >= blocker.bounds.max_x - kSupportFarFaceMargin) ||
        (move_x < -kFeetClearanceEpsilon &&
         final_box.min_x <= blocker.bounds.min_x + kSupportFarFaceMargin);
    const bool cleared_far_z =
        (move_z > kFeetClearanceEpsilon &&
         final_box.max_z >= blocker.bounds.max_z - kSupportFarFaceMargin) ||
        (move_z < -kFeetClearanceEpsilon &&
         final_box.min_z <= blocker.bounds.min_z + kSupportFarFaceMargin);
    const float probe_depth_x = overlap_depth_1d(probe_box.min_x, probe_box.max_x, blocker.bounds.min_x,
                                                 blocker.bounds.max_x);
    const float final_depth_x = overlap_depth_1d(final_box.min_x, final_box.max_x, blocker.bounds.min_x,
                                                 blocker.bounds.max_x);
    const float probe_depth_z = overlap_depth_1d(probe_box.min_z, probe_box.max_z, blocker.bounds.min_z,
                                                 blocker.bounds.max_z);
    const float final_depth_z = overlap_depth_1d(final_box.min_z, final_box.max_z, blocker.bounds.min_z,
                                                 blocker.bounds.max_z);
    const float probe_overlap_area = probe_depth_x * probe_depth_z;
    const float final_overlap_area = final_depth_x * final_depth_z;
    const bool moving = std::abs(move_x) > kFeetClearanceEpsilon ||
                        std::abs(move_z) > kFeetClearanceEpsilon;
    const bool overlap_shrinking =
        moving && final_overlap_area + kFeetClearanceEpsilon < probe_overlap_area;
    const bool overlap_too_shallow = final_overlap_area < kMinLandingOverlapArea;
    const float growth_x = final_depth_x - probe_depth_x;
    const float growth_z = final_depth_z - probe_depth_z;
    const bool strongly_dominant_x = std::abs(move_x) > std::abs(move_z) + kFeetClearanceEpsilon;
    const bool strongly_dominant_z = std::abs(move_z) > std::abs(move_x) + kFeetClearanceEpsilon;
    const bool through_motion =
        (strongly_dominant_x && std::abs(move_x) > kFeetClearanceEpsilon &&
         growth_x + kFeetClearanceEpsilon < std::abs(move_x) * 0.5f) ||
        (strongly_dominant_z && std::abs(move_z) > kFeetClearanceEpsilon &&
         growth_z + kFeetClearanceEpsilon < std::abs(move_z) * 0.5f);
    if (cleared_far_x || cleared_far_z || overlap_shrinking || through_motion ||
        overlap_too_shallow) {
      continue;
    }
    if (!best.has_value() || top > best->top_y + kFeetClearanceEpsilon) {
      best = SupportCandidate{i, top};
    }
  }
  return best;
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

SurfaceSample standing_sample(const SurfaceQuery& query, const CollisionWorld* solids, float x, float z,
                              float radius, float feet_y, float max_step_up) {
  if (solids == nullptr) {
    return query.sample(x, z);
  }
  SurfaceSample sample;
  (void)max_step_up;
  const std::optional<SolidSupport> support =
      query_solid_support(*solids, x, z, radius, feet_y, 1.0e6f);
  if (support.has_value()) {
    sample.y = support->y;
    sample.on_ramp = support->on_ramp;
    sample.ramp_index = support->on_ramp ? support->ramp_index : -1;
  }
  return sample;
}

void clamp_to_ceiling(PlayerBody& player, JumpState& jump, const CollisionWorld& world,
                      float ground_y) {
  CollisionBody body = collision_body_from_player(player);
  if (!cylinder_hits_ceiling(body, world)) {
    return;
  }
  float lowest = std::numeric_limits<float>::infinity();
  const float head = body.y + body.height;
  for (const WalkableBox& box : world.boxes) {
    const Aabb2 xz{box.min_x, box.min_z, box.max_x, box.max_z};
    if (!circle_overlaps_aabb2(body.x, body.z, body.radius, xz)) {
      continue;
    }
    if (body.y < box.y_lo && head > box.y_lo && body.y < box.y_hi) {
      lowest = std::min(lowest, box.y_lo);
    }
  }
  if (lowest == std::numeric_limits<float>::infinity()) {
    return;
  }
  const float max_feet = lowest - body.height;
  if (max_feet < ground_y) {
    player.y = ground_y;
    jump.vertical_speed = 0.0f;
    jump.jump_offset = 0.0f;
    jump.grounded = true;
    return;
  }
  if (player.y > max_feet) {
    player.y = max_feet;
    if (jump.vertical_speed > 0.0f) {
      jump.vertical_speed = 0.0f;
    }
    jump.jump_offset = std::max(0.0f, player.y - ground_y);
  }
}

struct JumpFrameCtx {
  PlayerBody& player;
  JumpState& jump;
  const PlayerFrameInput& input;
  float step_dt = 0.0f;
  std::span<const BlockerDef> blockers;
  const SurfaceQuery& surface_query;
  float max_step_up = 0.0f;
  std::span<const EdgeBarrierDef> edge_barriers;
  const MapData* map = nullptr;
  const CollisionWorld& collision_world;
  const CollisionWorld* solids = nullptr;
  float gravity_up = 0.0f;
  float gravity_down = 0.0f;
  float jump_speed = 0.0f;
  float jump_cut = 0.0f;
  float max_fall = 0.0f;
  float coyote_seconds = 0.0f;
  float input_buffer_seconds = 0.0f;
  float step_up_limit = 0.0f;
  float hop_speed = 0.0f;
  float bounce_speed = 0.0f;
  float lockout_seconds = 0.0f;
  bool jump_pressed_this_substep = false;
  bool was_grounded = false;
  bool was_climbing = false;
  bool& landed;
};

void latch_climb(JumpState& jump, bool was_climbing) {
  jump.climbing = true;
  if (!was_climbing) {
    jump.jump_buffer_left = 0.0f;
  }
}

void apply_ladder_bounce(JumpFrameCtx& ctx, const LadderVolume& ladder) {
  PlayerBody& player = ctx.player;
  JumpState& jump = ctx.jump;
  float ax = 0.0f;
  float az = 0.0f;
  ladder_face_away(ladder.face, ax, az);
  jump.grounded = false;
  jump.climbing = false;
  jump.coyote_time_left = 0.0f;
  jump.jump_buffer_left = 0.0f;
  jump.vertical_speed = ctx.hop_speed;
  jump.support_blocker_index = kInvalidSupportBlockerIndex;
  jump.ladder_bounce_x = ax;
  jump.ladder_bounce_z = az;
  jump.ladder_lockout_left = ctx.lockout_seconds;
  const SurfaceSample air0 =
      standing_sample(ctx.surface_query, ctx.solids, player.x, player.z, player.half_extent,
                      player.y, ctx.step_up_limit);
  jump.jump_offset = std::max(0.0f, player.y - air0.y);
  try_move_xz(player, ax * kLadderBounceNudge, az * kLadderBounceNudge, ctx.blockers,
              ctx.collision_world, ctx.step_up_limit);
}

// Climb motor owns the whole substep: stay latched, snap to a slab, or walk off into air.
void tick_climb_substep(JumpFrameCtx& ctx) {
  PlayerBody& player = ctx.player;
  JumpState& jump = ctx.jump;
  player = integrate_player_surface(player, ctx.input.move, ctx.step_dt, ctx.blockers,
                                    ctx.surface_query, ctx.max_step_up, ctx.edge_barriers, ctx.map,
                                    false, ctx.input.climb_move);
  if (overlapping_ladder(collision_body_from_player(player), ctx.collision_world) != nullptr) {
    jump.grounded = true;
    jump.vertical_speed = 0.0f;
    jump.jump_offset = 0.0f;
    jump.coyote_time_left = ctx.coyote_seconds;
    jump.support_blocker_index = kInvalidSupportBlockerIndex;
    latch_climb(jump, ctx.was_climbing);
    if (!ctx.was_grounded) {
      ctx.landed = true;
    }
    return;
  }
  const std::optional<SolidSupport> ladder_support =
      query_solid_support(ctx.collision_world, player.x, player.z, player.half_extent, player.y,
                          ctx.step_up_limit);
  if (ladder_support.has_value()) {
    player.y = ladder_support->y;
    jump.grounded = true;
    jump.vertical_speed = 0.0f;
    jump.jump_offset = 0.0f;
    jump.coyote_time_left = ctx.coyote_seconds;
    jump.support_blocker_index = kInvalidSupportBlockerIndex;
    if (!ctx.was_grounded) {
      ctx.landed = true;
    }
    return;
  }
  jump.grounded = false;
  jump.vertical_speed = 0.0f;
  jump.support_blocker_index = kInvalidSupportBlockerIndex;
  const SurfaceSample air = standing_sample(ctx.surface_query, ctx.solids, player.x, player.z,
                                            player.half_extent, player.y, ctx.step_up_limit);
  jump.jump_offset = std::max(0.0f, player.y - air.y);
  jump.coyote_time_left = ctx.coyote_seconds;
  jump.vertical_speed -= ctx.gravity_down * ctx.step_dt;
  jump.vertical_speed = std::max(jump.vertical_speed, -ctx.max_fall);
  jump.jump_offset += jump.vertical_speed * ctx.step_dt;
  if (jump.jump_offset <= 0.0f) {
    jump.jump_offset = 0.0f;
    jump.vertical_speed = 0.0f;
    jump.grounded = true;
    player.y = air.y;
    if (!ctx.was_grounded) {
      ctx.landed = true;
    }
  } else {
    player.y = air.y + jump.jump_offset;
  }
}

void move_grounded_xz(JumpFrameCtx& ctx, const std::optional<SupportCandidate>& support_before) {
  PlayerBody& player = ctx.player;
  if (support_before.has_value()) {
    player = integrate_player(player, ctx.input.move, ctx.step_dt, ctx.blockers, ctx.edge_barriers,
                              &ctx.surface_query, ctx.map);
  } else {
    player = integrate_player_surface(player, ctx.input.move, ctx.step_dt, ctx.blockers,
                                      ctx.surface_query, ctx.max_step_up, ctx.edge_barriers, ctx.map,
                                      ctx.jump.ladder_lockout_left > 1e-6f, ctx.input.climb_move);
  }
}

void move_airborne_xz(JumpFrameCtx& ctx, float feet_world_before) {
  PlayerBody& player = ctx.player;
  constexpr float kAirborneTerrainProbe = 1.0e6f;
  float ix = ctx.input.move.axis_x;
  float iz = ctx.input.move.axis_z;
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
    player = integrate_player(player, move_x, ctx.step_dt * std::abs(ix), ctx.blockers,
                              ctx.edge_barriers, &ctx.surface_query, ctx.map);
    const SurfaceSample candidate_x = standing_sample(
        ctx.surface_query, ctx.solids, player.x, player.z, player.half_extent, feet_world_before,
        ctx.solids != nullptr ? kAirborneTerrainProbe : ctx.step_up_limit);
    if (candidate_x.y > feet_world_before + kFeetPenetrationEpsilon) {
      player = before_x;
    }
  }

  if (std::abs(iz) > 1e-6f) {
    const PlayerBody before_z = player;
    const MoveInput move_z{0.0f, iz > 0.0f ? 1.0f : -1.0f};
    player = integrate_player(player, move_z, ctx.step_dt * std::abs(iz), ctx.blockers,
                              ctx.edge_barriers, &ctx.surface_query, ctx.map);
    const SurfaceSample candidate_z = standing_sample(
        ctx.surface_query, ctx.solids, player.x, player.z, player.half_extent, feet_world_before,
        ctx.solids != nullptr ? kAirborneTerrainProbe : ctx.step_up_limit);
    if (candidate_z.y > feet_world_before + kFeetPenetrationEpsilon) {
      player = before_z;
    }
  }
}

// Idle/Walk and Jump/Fall share one integrator: a substep can walk off a ledge or land.
void tick_ground_air_substep(JumpFrameCtx& ctx) {
  PlayerBody& player = ctx.player;
  JumpState& jump = ctx.jump;
  const PlayerFrameInput& input = ctx.input;
  const float step_dt = ctx.step_dt;
  std::span<const BlockerDef> blockers = ctx.blockers;
  const SurfaceQuery& surface_query = ctx.surface_query;
  const float max_step_up = ctx.max_step_up;
  const CollisionWorld& collision_world = ctx.collision_world;
  const CollisionWorld* solids = ctx.solids;
  const float gravity_up = ctx.gravity_up;
  const float gravity_down = ctx.gravity_down;
  const float jump_speed = ctx.jump_speed;
  const float jump_cut = ctx.jump_cut;
  const float max_fall = ctx.max_fall;
  const float coyote_seconds = ctx.coyote_seconds;
  const float input_buffer_seconds = ctx.input_buffer_seconds;
  const float step_up_limit = ctx.step_up_limit;
  const float bounce_speed = ctx.bounce_speed;
  const bool jump_pressed_this_substep = ctx.jump_pressed_this_substep;
  const bool was_grounded = ctx.was_grounded;

  if (jump.ladder_lockout_left > 1e-6f) {
    jump.ladder_lockout_left = std::max(0.0f, jump.ladder_lockout_left - step_dt);
    try_move_xz(player, jump.ladder_bounce_x * bounce_speed * step_dt,
                jump.ladder_bounce_z * bounce_speed * step_dt, blockers, collision_world,
                step_up_limit);
    if (jump.ladder_lockout_left <= 1e-6f) {
      jump.ladder_lockout_left = 0.0f;
      jump.ladder_bounce_x = 0.0f;
      jump.ladder_bounce_z = 0.0f;
    }
  }
  bool walked_off_drop = false;
  const SurfaceSample sample_before = standing_sample(
      surface_query, solids, player.x, player.z, player.half_extent, player.y, step_up_limit);
  float feet_world_before = jump.grounded ? player.y : (sample_before.y + clamp_nonnegative(jump.jump_offset));
  feet_world_before = std::max(feet_world_before, sample_before.y);
  std::optional<SupportCandidate> support_before;
  if (jump.grounded) {
    support_before =
        validate_active_support(player, blockers, jump.support_blocker_index, feet_world_before);
    if (!support_before.has_value()) {
      support_before = resolve_support_for_feet_world(player, blockers, feet_world_before);
      if (support_before.has_value()) {
        jump.support_blocker_index = support_before->index;
        feet_world_before = support_before->top_y;
      } else {
        jump.support_blocker_index = kInvalidSupportBlockerIndex;
        if (feet_world_before > sample_before.y + kFeetClearanceEpsilon) {
          jump.grounded = false;
          jump.vertical_speed = 0.0f;
          jump.jump_offset = feet_world_before - sample_before.y;
          jump.coyote_time_left = coyote_seconds;
          walked_off_drop = true;
        }
      }
    } else {
      feet_world_before = support_before->top_y;
    }
  }

  player.y = feet_world_before;
  const PlayerBody before_move = player;

  const float predicted_offset_after_step =
      predict_feet_world_after_step(jump, input.jump_held, step_dt, gravity_up, gravity_down, jump_speed,
                                    jump_cut, max_fall);
  const float feet_world_predicted_after =
      jump.grounded ? feet_world_before : (sample_before.y + std::max(0.0f, predicted_offset_after_step));

  if (jump.grounded) {
    move_grounded_xz(ctx, support_before);
  } else {
    move_airborne_xz(ctx, feet_world_before);
  }
  SurfaceSample sample_after = standing_sample(surface_query, solids, player.x, player.z,
                                               player.half_extent, player.y, step_up_limit);
  float ground_y = sample_after.y;
  if (!jump.grounded) {
    const std::optional<SupportCandidate> landing = find_descending_support_landing(
        before_move, player, blockers, feet_world_before, feet_world_predicted_after);
    if (landing.has_value()) {
      jump.grounded = true;
      jump.vertical_speed = 0.0f;
      jump.jump_offset = 0.0f;
      jump.coyote_time_left = coyote_seconds;
      jump.support_blocker_index = landing->index;
      player.y = landing->top_y;
    }
  }

  std::optional<SupportCandidate> support_after;
  bool support_from_overlap_resolve = false;
  if (jump.grounded) {
    support_after = validate_active_support(player, blockers, jump.support_blocker_index, player.y);
    if (!support_after.has_value()) {
      support_after = resolve_support_for_feet_world(player, blockers, player.y);
      if (support_after.has_value()) {
        support_from_overlap_resolve = true;
        jump.support_blocker_index = support_after->index;
      } else {
        jump.support_blocker_index = kInvalidSupportBlockerIndex;
      }
    }
  }
  if (jump.grounded && support_from_overlap_resolve && support_after.has_value()) {
    const BlockerDef& blocker = blockers[static_cast<std::size_t>(support_after->index)];
    const Aabb2 before_box = body_bounds(before_move);
    const Aabb2 after_box = body_bounds(player);
    const float move_x = player.x - before_move.x;
    const float move_z = player.z - before_move.z;
    const bool cleared_far_x =
        (move_x > kFeetClearanceEpsilon &&
         after_box.max_x >= blocker.bounds.max_x - kSupportFarFaceMargin) ||
        (move_x < -kFeetClearanceEpsilon &&
         after_box.min_x <= blocker.bounds.min_x + kSupportFarFaceMargin);
    const bool cleared_far_z =
        (move_z > kFeetClearanceEpsilon &&
         after_box.max_z >= blocker.bounds.max_z - kSupportFarFaceMargin) ||
        (move_z < -kFeetClearanceEpsilon &&
         after_box.min_z <= blocker.bounds.min_z + kSupportFarFaceMargin);
    const float before_depth_x = overlap_depth_1d(before_box.min_x, before_box.max_x,
                                                  blocker.bounds.min_x, blocker.bounds.max_x);
    const float after_depth_x = overlap_depth_1d(after_box.min_x, after_box.max_x,
                                                 blocker.bounds.min_x, blocker.bounds.max_x);
    const float before_depth_z = overlap_depth_1d(before_box.min_z, before_box.max_z,
                                                  blocker.bounds.min_z, blocker.bounds.max_z);
    const float after_depth_z = overlap_depth_1d(after_box.min_z, after_box.max_z,
                                                 blocker.bounds.min_z, blocker.bounds.max_z);
    const float before_overlap_area = before_depth_x * before_depth_z;
    const float after_overlap_area = after_depth_x * after_depth_z;
    const bool overlap_shrinking =
        after_overlap_area + kFeetClearanceEpsilon < before_overlap_area;
    const bool moving = std::abs(move_x) > kFeetClearanceEpsilon ||
                        std::abs(move_z) > kFeetClearanceEpsilon;
    const float near_pen_x = std::min(after_box.max_x - blocker.bounds.min_x,
                                      blocker.bounds.max_x - after_box.min_x);
    const float far_pen_x = std::max(after_box.max_x - blocker.bounds.min_x,
                                     blocker.bounds.max_x - after_box.min_x);
    const float near_pen_z = std::min(after_box.max_z - blocker.bounds.min_z,
                                      blocker.bounds.max_z - after_box.min_z);
    const float far_pen_z = std::max(after_box.max_z - blocker.bounds.min_z,
                                     blocker.bounds.max_z - after_box.min_z);
    const bool static_skewed_overlap =
        !moving &&
        ((near_pen_x + kFeetClearanceEpsilon < far_pen_x * 0.5f) ||
         (near_pen_z + kFeetClearanceEpsilon < far_pen_z * 0.5f));
    if (cleared_far_x || cleared_far_z || overlap_shrinking || static_skewed_overlap) {
      support_after.reset();
      jump.support_blocker_index = kInvalidSupportBlockerIndex;
    }
  }

  if (jump.grounded && support_before.has_value() && !support_after.has_value()) {
    const float rise_above_support = ground_y - feet_world_before;
    if (rise_above_support > kFeetClearanceEpsilon) {
      const bool reachable_step =
          sample_after.on_ramp || sample_before.on_ramp ||
          rise_above_support <= std::max(0.0f, max_step_up);
      if (reachable_step) {
        jump.support_blocker_index = kInvalidSupportBlockerIndex;
      } else {
        player = before_move;
        sample_after = standing_sample(surface_query, solids, player.x, player.z,
                                       player.half_extent, player.y, step_up_limit);
        ground_y = sample_after.y;
        jump.support_blocker_index = support_before->index;
        support_after = validate_active_support(player, blockers, jump.support_blocker_index,
                                                feet_world_before);
      }
    } else if (feet_world_before > ground_y + kFeetClearanceEpsilon) {
      jump.grounded = false;
      jump.vertical_speed = 0.0f;
      jump.jump_offset = std::max(0.0f, feet_world_before - ground_y);
      jump.coyote_time_left = coyote_seconds;
      jump.support_blocker_index = kInvalidSupportBlockerIndex;
      walked_off_drop = true;
    } else {
      jump.support_blocker_index = kInvalidSupportBlockerIndex;
    }
  }

  if (jump.grounded && support_after.has_value()) {
    if (ground_y > support_after->top_y + kFeetClearanceEpsilon) {
      const float rise = ground_y - support_after->top_y;
      const bool reachable_step =
          sample_after.on_ramp || sample_before.on_ramp || rise <= std::max(0.0f, max_step_up);
      if (reachable_step) {
        jump.support_blocker_index = kInvalidSupportBlockerIndex;
        support_after.reset();
      } else {
        player = before_move;
        sample_after = standing_sample(surface_query, solids, player.x, player.z,
                                       player.half_extent, player.y, step_up_limit);
        ground_y = sample_after.y;
        support_after = validate_active_support(player, blockers, jump.support_blocker_index, feet_world_before);
      }
    }
    if (support_after.has_value()) {
      const BlockerDef& blocker = blockers[static_cast<std::size_t>(support_after->index)];
      const bool on_support = overlaps_footprint(player, blocker);
      if (!on_support) {
        if (ground_y >= support_after->top_y - kFeetClearanceEpsilon) {
          jump.support_blocker_index = kInvalidSupportBlockerIndex;
          support_after.reset();
        } else {
          jump.grounded = false;
          jump.vertical_speed = 0.0f;
          jump.jump_offset = std::max(0.0f, support_after->top_y - ground_y);
          jump.coyote_time_left = coyote_seconds;
          jump.support_blocker_index = kInvalidSupportBlockerIndex;
          support_after.reset();
          walked_off_drop = true;
        }
      } else if (ground_y >= support_after->top_y - kFeetClearanceEpsilon) {
        jump.support_blocker_index = kInvalidSupportBlockerIndex;
        support_after.reset();
      }
    }
  }

  if (!jump.grounded) {
    jump.jump_offset = std::max(0.0f, feet_world_before - ground_y);
  } else if (!support_after.has_value() && was_grounded && jump.jump_offset <= 0.0f &&
             !sample_after.on_ramp &&
             sample_after.y < sample_before.y - kWalkOffDropEpsilon) {
    jump.grounded = false;
    jump.vertical_speed = 0.0f;
    jump.jump_offset = sample_before.y - sample_after.y;
    jump.coyote_time_left = coyote_seconds;
    walked_off_drop = true;
  }

  if (jump.ladder_lockout_left > 1e-6f) {
    jump.jump_buffer_left = std::max(0.0f, jump.jump_buffer_left - step_dt);
  } else if (jump_pressed_this_substep) {
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
    if (jump.grounded) {
      const float support_ground = support_after.has_value() ? support_after->top_y : ground_y;
      jump.jump_offset = std::max(0.0f, support_ground - ground_y);
    }
    launch_jump(jump, jump_speed);
    support_after.reset();
  }

  if (!input.jump_held && jump.vertical_speed > 0.0f && jump.ladder_lockout_left <= 1e-6f) {
    jump.vertical_speed = std::min(jump.vertical_speed, jump_speed * jump_cut);
  }

  if ((!jump.grounded || jump.jump_offset > 0.0f || jump.vertical_speed != 0.0f) &&
      !walked_off_drop) {
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
  } else {
    jump.support_blocker_index = kInvalidSupportBlockerIndex;
  }

  const float effective_ground_y = support_after.has_value() ? support_after->top_y : ground_y;
  player.y = effective_ground_y + std::max(0.0f, jump.jump_offset);
  if (solids != nullptr) {
    clamp_to_ceiling(player, jump, *solids, effective_ground_y);
  }

  if (jump.grounded) {
    depenetrate_grounded_overlap(player, blockers, effective_ground_y, input.move.axis_x, input.move.axis_z);
    if (!sample_after.on_ramp) {
      CollisionBody wall_body = collision_body_from_player(player);
      depenetrate_cylinder_from_walls(wall_body, collision_world, std::max(0.0f, max_step_up));
      player.x = wall_body.x;
      player.z = wall_body.z;
    }
    if (support_after.has_value()) {
      const float support_world_y = support_after->top_y;
      const std::optional<SupportCandidate> corrected_support = validate_active_support(
          player, blockers, jump.support_blocker_index, support_world_y);
      if (corrected_support.has_value()) {
        player.y = corrected_support->top_y;
      } else {
        const SurfaceSample corrected =
            standing_sample(surface_query, solids, player.x, player.z, player.half_extent,
                            player.y, step_up_limit);
        if (corrected.y < support_world_y - kFeetClearanceEpsilon) {
          jump.grounded = false;
          jump.vertical_speed = 0.0f;
          jump.jump_offset = std::max(0.0f, support_world_y - corrected.y);
          jump.coyote_time_left = coyote_seconds;
          jump.support_blocker_index = kInvalidSupportBlockerIndex;
          player.y = support_world_y;
        } else {
          jump.support_blocker_index = kInvalidSupportBlockerIndex;
          player.y = corrected.y;
        }
      }
    } else {
      const SurfaceSample corrected =
          standing_sample(surface_query, solids, player.x, player.z, player.half_extent,
                          player.y, step_up_limit);
      player.y = corrected.y;
    }
  }

  if (!was_grounded && jump.grounded) {
    ctx.landed = true;  // sticky for the frame; a later substep takeoff does not clear this
  }
  if (!jump.climbing && jump.ladder_lockout_left <= 1e-6f && jump.grounded &&
      overlapping_ladder(collision_body_from_player(player), collision_world) != nullptr) {
    latch_climb(jump, ctx.was_climbing);
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
  state.support_blocker_index = kInvalidSupportBlockerIndex;
  state.ladder_lockout_left = 0.0f;
  state.ladder_bounce_x = 0.0f;
  state.ladder_bounce_z = 0.0f;
  state.climbing = false;
  return state;
}

PlayerFrameResult integrate_player_frame_surface(PlayerBody player, JumpState jump,
                                                 const PlayerFrameInput& input, float dt,
                                                 std::span<const BlockerDef> blockers,
                                                 const SurfaceQuery& surface_query,
                                                 const JumpTuning& tuning, float max_step_up,
                                                 std::span<const EdgeBarrierDef> edge_barriers,
                                                 const MapData* map) {
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
  const float step_up_limit = std::max(0.0f, max_step_up);
  const CollisionWorld collision_world =
      map != nullptr ? bake_collision_world(*map, surface_query)
                     : bake_fence_world(edge_barriers, surface_query);
  const CollisionWorld* solids = map != nullptr ? &collision_world : nullptr;
  const float hop_speed = clamp_nonnegative(tuning.ladder_hop_speed);
  const float bounce_speed = clamp_nonnegative(tuning.ladder_bounce_speed);
  const float lockout_seconds = clamp_nonnegative(tuning.ladder_lockout_seconds);

  bool landed = false;
  for (int i = 0; i < substeps; ++i) {
    const bool jump_pressed_this_substep = input.jump_pressed && i == 0;
    const bool was_grounded = jump.grounded;
    const bool was_climbing = jump.climbing;
    jump.climbing = false;
    const bool lockout_active = jump.ladder_lockout_left > 1e-6f;
    const LadderVolume* ladder =
        lockout_active ? nullptr
                       : overlapping_ladder(collision_body_from_player(player), collision_world);

    JumpFrameCtx ctx{
        player,
        jump,
        input,
        step_dt,
        blockers,
        surface_query,
        max_step_up,
        edge_barriers,
        map,
        collision_world,
        solids,
        gravity_up,
        gravity_down,
        jump_speed,
        jump_cut,
        max_fall,
        coyote_seconds,
        input_buffer_seconds,
        step_up_limit,
        hop_speed,
        bounce_speed,
        lockout_seconds,
        jump_pressed_this_substep,
        was_grounded,
        was_climbing,
        landed,
    };

    LocomotionState motor = LocomotionState::Idle;
    if (ladder != nullptr) {
      const bool want_bounce =
          jump_pressed_this_substep || (was_climbing && jump.jump_buffer_left > 1e-6f);
      if (want_bounce) {
        apply_ladder_bounce(ctx, *ladder);
        motor = locomotion_from(jump, input.move);
      } else {
        motor = LocomotionState::Climb;
      }
    } else {
      motor = locomotion_from(jump, input.move);
    }

    switch (motor) {
      case LocomotionState::Climb:
        tick_climb_substep(ctx);
        break;
      case LocomotionState::Idle:
      case LocomotionState::Walk:
      case LocomotionState::Jump:
      case LocomotionState::Fall:
        tick_ground_air_substep(ctx);
        break;
    }
  }

  return PlayerFrameResult{player, jump, landed};
}

}  // namespace rat
