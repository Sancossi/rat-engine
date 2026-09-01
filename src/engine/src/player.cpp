#include "rat/player.hpp"
#include "rat/collision.hpp"
#include "rat/map_data.hpp"
#include "rat/surface_query.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace rat {
namespace {

Aabb2 player_bounds(const PlayerBody& player) {
  return {player.x - player.half_extent, player.z - player.half_extent,
          player.x + player.half_extent, player.z + player.half_extent};
}

constexpr float kFeetClearanceEpsilon = 1e-4f;

bool overlaps_any(const Aabb2& box, std::span<const BlockerDef> blockers, float feet_world_y) {
  for (const BlockerDef& blocker : blockers) {
    if (aabb_overlap(box, blocker.bounds) &&
        blocker_blocks_feet(blocker, feet_world_y, kFeetClearanceEpsilon)) {
      return true;
    }
  }
  return false;
}

bool surface_step_allowed(const SurfaceSample& from, const SurfaceSample& to, float max_step_up) {
  const float rise = to.y - from.y;
  if (rise <= 0.0f) {
    return true;
  }
  if (from.on_ramp && to.on_ramp && from.ramp_index >= 0 && from.ramp_index == to.ramp_index) {
    return true;
  }
  return rise <= max_step_up;
}

CollisionWorld bake_integrate_world(std::span<const EdgeBarrierDef> edge_barriers,
                                    const SurfaceQuery& query, const MapData* map) {
  if (map != nullptr) {
    return bake_collision_world(*map, query);
  }
  return bake_fence_world(edge_barriers, query);
}

SurfaceSample standing_sample(const SurfaceQuery& surface_query, const CollisionWorld& world,
                              const MapData* map, float x, float z, float radius, float feet_y,
                              float max_step_up) {
  if (map == nullptr) {
    return surface_query.sample(x, z);
  }
  SurfaceSample sample;
  // Uncapped step so the current cell's top matches SurfaceQuery (ramp snap from y=0).
  // Movement still applies max_step_up via surface_step_allowed.
  (void)max_step_up;
  const std::optional<SolidSupport> support =
      query_solid_support(world, x, z, radius, feet_y, 1.0e6f);
  if (support.has_value()) {
    sample.y = support->y;
    sample.on_ramp = support->on_ramp;
    sample.ramp_index = support->on_ramp ? 0 : -1;
  }
  return sample;
}

}  // namespace

bool aabb_overlap(const Aabb2& a, const Aabb2& b) {
  return a.min_x < b.max_x && a.max_x > b.min_x && a.min_z < b.max_z && a.max_z > b.min_z;
}

MoveInput camera_relative_move(float screen_x, float screen_z, Vec3 eye, Vec3 focus) {
  // Ground-projected look direction (away from camera, into the scene).
  float fx = focus.x - eye.x;
  float fz = focus.z - eye.z;
  const float flen = std::sqrt(fx * fx + fz * fz);
  if (flen <= 1e-6f) {
    // Top-down (eye directly above, view up = -Z): screen up = -Z, screen right = -X
    // (matches left-handed bgfx look-at used at draw time).
    MoveInput out;
    out.axis_x = -screen_x;
    out.axis_z = -screen_z;
    return out;
  }
  fx /= flen;
  fz /= flen;
  // Screen-right on XZ: rotate look 90° clockwise when viewed from +Y.
  const float rx = fz;
  const float rz = -fx;

  MoveInput out;
  out.axis_x = fx * screen_z + rx * screen_x;
  out.axis_z = fz * screen_z + rz * screen_x;
  return out;
}

MoveInput world_aligned_move(float screen_x, float screen_z) {
  return MoveInput{-screen_x, -screen_z};
}

PlayerBody integrate_player(PlayerBody player, MoveInput input, float dt,
                            std::span<const BlockerDef> blockers,
                            std::span<const EdgeBarrierDef> edge_barriers,
                            const SurfaceQuery* surface_query, const MapData* map) {
  float ix = input.axis_x;
  float iz = input.axis_z;
  const float len = std::sqrt(ix * ix + iz * iz);
  if (len > 1e-6f) {
    ix /= len;
    iz /= len;
  }

  const float dx = ix * player.speed * dt;
  const float dz = iz * player.speed * dt;
  const float max_step = std::max(0.05f, player.half_extent * 0.5f);
  const int steps =
      std::max(1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dz)) / max_step)));
  const float step_dx = dx / static_cast<float>(steps);
  const float step_dz = dz / static_cast<float>(steps);
  CollisionWorld world;
  if (surface_query != nullptr) {
    world = bake_integrate_world(edge_barriers, *surface_query, map);
  }

  for (int i = 0; i < steps; ++i) {
    const float old_x = player.x;
    player.x += step_dx;
    if (overlaps_any(player_bounds(player), blockers, player.y) ||
        (surface_query != nullptr &&
         cylinder_hits_walls(collision_body_from_player(player), world, 0.35f))) {
      player.x = old_x;
    }

    const float old_z = player.z;
    player.z += step_dz;
    if (overlaps_any(player_bounds(player), blockers, player.y) ||
        (surface_query != nullptr &&
         cylinder_hits_walls(collision_body_from_player(player), world, 0.35f))) {
      player.z = old_z;
    }
  }

  return player;
}

PlayerBody integrate_player_surface(PlayerBody player, MoveInput input, float dt,
                                    std::span<const BlockerDef> blockers,
                                    const SurfaceQuery& surface_query, float max_step_up,
                                    std::span<const EdgeBarrierDef> edge_barriers,
                                    const MapData* map) {
  float ix = input.axis_x;
  float iz = input.axis_z;
  const float len = std::sqrt(ix * ix + iz * iz);
  if (len > 1e-6f) {
    ix /= len;
    iz /= len;
  }

  const float dx = ix * player.speed * dt;
  const float dz = iz * player.speed * dt;
  const float max_step = std::max(0.05f, player.half_extent * 0.5f);
  const int steps =
      std::max(1, static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dz)) / max_step)));
  const float step_dx = dx / static_cast<float>(steps);
  const float step_dz = dz / static_cast<float>(steps);
  const float step_up_limit = std::max(0.0f, max_step_up);
  const CollisionWorld world = bake_integrate_world(edge_barriers, surface_query, map);
  SurfaceSample current_sample =
      standing_sample(surface_query, world, map, player.x, player.z, player.half_extent, player.y,
                      step_up_limit);
  player.y = current_sample.y;

  // Keep deterministic axis slide semantics: resolve X, then resolve Z per substep.
  for (int i = 0; i < steps; ++i) {
    if (std::abs(step_dx) > 1e-6f) {
      const float old_x = player.x;
      player.x += step_dx;
      const SurfaceSample sample =
          standing_sample(surface_query, world, map, player.x, player.z, player.half_extent,
                          player.y, step_up_limit);
      const bool step_ok = surface_step_allowed(current_sample, sample, step_up_limit);
      CollisionBody wall_body = collision_body_from_player(player);
      if (step_ok) {
        // Walk-off: keep current feet so the ledge/trapezoid top is skippable.
        wall_body.y = std::max(player.y, sample.y);
        if (current_sample.on_ramp || sample.on_ramp) {
          wall_body.y += step_up_limit;
        }
      }
      if (!step_ok || overlaps_any(player_bounds(player), blockers, player.y) ||
          cylinder_hits_walls(wall_body, world, step_up_limit)) {
        player.x = old_x;
      } else {
        current_sample = sample;
        player.y = sample.y;
      }
    }

    if (std::abs(step_dz) > 1e-6f) {
      const float old_z = player.z;
      player.z += step_dz;
      const SurfaceSample sample =
          standing_sample(surface_query, world, map, player.x, player.z, player.half_extent,
                          player.y, step_up_limit);
      const bool step_ok = surface_step_allowed(current_sample, sample, step_up_limit);
      CollisionBody wall_body = collision_body_from_player(player);
      if (step_ok) {
        wall_body.y = std::max(player.y, sample.y);
        if (current_sample.on_ramp || sample.on_ramp) {
          wall_body.y += step_up_limit;
        }
      }
      if (!step_ok || overlaps_any(player_bounds(player), blockers, player.y) ||
          cylinder_hits_walls(wall_body, world, step_up_limit)) {
        player.z = old_z;
      } else {
        current_sample = sample;
        player.y = sample.y;
      }
    }
  }

  return player;
}

Vec3 snap_to_grid(float x, float y, float z, float tile_size) {
  const float t = std::max(1e-6f, tile_size);
  // Snap to cell centers (…, -0.5, 0.5, 1.5, …), not grid-line intersections.
  auto snap_center = [t](float v) { return std::floor(v / t) * t + 0.5f * t; };
  return {snap_center(x), y, snap_center(z)};
}

}  // namespace rat
