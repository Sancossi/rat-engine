#include "rat/player.hpp"
#include "rat/map_data.hpp"
#include "rat/surface_query.hpp"

#include <algorithm>
#include <cmath>

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

bool ranges_overlap(float min_a, float max_a, float min_b, float max_b) {
  return min_a < max_b && max_a > min_b;
}

bool blocked_by_edge_barriers(float old_x, float old_z, float new_x, float new_z, float feet_y,
                              float half_extent, std::span<const EdgeBarrierDef> barriers,
                              const SurfaceQuery& query) {
  if (barriers.empty()) {
    return false;
  }
  const float ts = query.tile_size() > 0.0f ? query.tile_size() : 1.0f;
  const float z_lo = std::min(old_z, new_z) - half_extent;
  const float z_hi = std::max(old_z, new_z) + half_extent;
  const float x_lo = std::min(old_x, new_x) - half_extent;
  const float x_hi = std::max(old_x, new_x) + half_extent;

  for (const EdgeBarrierDef& edge : barriers) {
    if (edge.height <= 0.0f) {
      continue;
    }
    const float tile_x0 = static_cast<float>(edge.tile.x) * ts;
    const float tile_z0 = static_cast<float>(edge.tile.z) * ts;
    const float tile_x1 = tile_x0 + ts;
    const float tile_z1 = tile_z0 + ts;
    const float owner_top = query.sample(tile_x0 + 0.5f * ts, tile_z0 + 0.5f * ts).y;
    if (feet_y + kFeetClearanceEpsilon >= owner_top + edge.height) {
      continue;
    }

    switch (edge.direction) {
      case RampDirection::East:
      case RampDirection::West: {
        const float edge_x = edge.direction == RampDirection::East ? tile_x1 : tile_x0;
        if ((old_x < edge_x) == (new_x < edge_x)) {
          break;
        }
        if (!ranges_overlap(z_lo, z_hi, tile_z0, tile_z1)) {
          break;
        }
        return true;
      }
      case RampDirection::North:
      case RampDirection::South: {
        const float edge_z = edge.direction == RampDirection::South ? tile_z1 : tile_z0;
        if ((old_z < edge_z) == (new_z < edge_z)) {
          break;
        }
        if (!ranges_overlap(x_lo, x_hi, tile_x0, tile_x1)) {
          break;
        }
        return true;
      }
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
                            const SurfaceQuery* surface_query) {
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

  for (int i = 0; i < steps; ++i) {
    const float old_x = player.x;
    player.x += step_dx;
    if (overlaps_any(player_bounds(player), blockers, player.y) ||
        (surface_query != nullptr &&
         blocked_by_edge_barriers(old_x, player.z, player.x, player.z, player.y, player.half_extent,
                                  edge_barriers, *surface_query))) {
      player.x = old_x;
    }

    const float old_z = player.z;
    player.z += step_dz;
    if (overlaps_any(player_bounds(player), blockers, player.y) ||
        (surface_query != nullptr &&
         blocked_by_edge_barriers(player.x, old_z, player.x, player.z, player.y, player.half_extent,
                                  edge_barriers, *surface_query))) {
      player.z = old_z;
    }
  }

  return player;
}

PlayerBody integrate_player_surface(PlayerBody player, MoveInput input, float dt,
                                    std::span<const BlockerDef> blockers,
                                    const SurfaceQuery& surface_query, float max_step_up,
                                    std::span<const EdgeBarrierDef> edge_barriers) {
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
  SurfaceSample current_sample = surface_query.sample(player.x, player.z);
  player.y = current_sample.y;

  // Keep deterministic axis slide semantics: resolve X, then resolve Z per substep.
  for (int i = 0; i < steps; ++i) {
    if (std::abs(step_dx) > 1e-6f) {
      const float old_x = player.x;
      player.x += step_dx;
      if (overlaps_any(player_bounds(player), blockers, player.y) ||
          blocked_by_edge_barriers(old_x, player.z, player.x, player.z, player.y, player.half_extent,
                                   edge_barriers, surface_query)) {
        player.x = old_x;
      } else {
        const SurfaceSample sample = surface_query.sample(player.x, player.z);
        if (surface_step_allowed(current_sample, sample, step_up_limit)) {
          current_sample = sample;
          player.y = sample.y;
        } else {
          player.x = old_x;
          player.y = current_sample.y;
        }
      }
    }

    if (std::abs(step_dz) > 1e-6f) {
      const float old_z = player.z;
      player.z += step_dz;
      if (overlaps_any(player_bounds(player), blockers, player.y) ||
          blocked_by_edge_barriers(player.x, old_z, player.x, player.z, player.y, player.half_extent,
                                   edge_barriers, surface_query)) {
        player.z = old_z;
      } else {
        const SurfaceSample sample = surface_query.sample(player.x, player.z);
        if (surface_step_allowed(current_sample, sample, step_up_limit)) {
          current_sample = sample;
          player.y = sample.y;
        } else {
          player.z = old_z;
          player.y = current_sample.y;
        }
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
