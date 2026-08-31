#include "rat/player.hpp"

#include <algorithm>
#include <cmath>

namespace rat {
namespace {

Aabb2 player_bounds(const PlayerBody& player) {
  return {player.x - player.half_extent, player.z - player.half_extent,
          player.x + player.half_extent, player.z + player.half_extent};
}

bool overlaps_any(const Aabb2& box, std::span<const Aabb2> blockers) {
  for (const Aabb2& b : blockers) {
    if (aabb_overlap(box, b)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool aabb_overlap(const Aabb2& a, const Aabb2& b) {
  return a.min_x < b.max_x && a.max_x > b.min_x && a.min_z < b.max_z && a.max_z > b.min_z;
}

PlayerBody integrate_player(PlayerBody player, MoveInput input, float dt,
                            std::span<const Aabb2> blockers) {
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
    if (overlaps_any(player_bounds(player), blockers)) {
      player.x = old_x;
    }

    const float old_z = player.z;
    player.z += step_dz;
    if (overlaps_any(player_bounds(player), blockers)) {
      player.z = old_z;
    }
  }

  return player;
}

Vec3 snap_to_grid(float x, float y, float z, float tile_size) {
  const float t = std::max(1e-6f, tile_size);
  auto snap = [t](float v) { return std::round(v / t) * t; };
  return {snap(x), y, snap(z)};
}

}  // namespace rat
