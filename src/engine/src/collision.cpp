#include "rat/collision.hpp"

#include <algorithm>

namespace rat {
namespace {

bool ranges_overlap(float min_a, float max_a, float min_b, float max_b) {
  return min_a < max_b && max_a > min_b;
}

bool circle_hits_segment(float cx, float cz, float radius, float ax, float az, float bx, float bz) {
  const float abx = bx - ax;
  const float abz = bz - az;
  const float acx = cx - ax;
  const float acz = cz - az;
  const float ab_len2 = abx * abx + abz * abz;
  float t = 0.0f;
  if (ab_len2 > 0.0f) {
    t = std::clamp((acx * abx + acz * abz) / ab_len2, 0.0f, 1.0f);
  }
  const float dx = cx - (ax + t * abx);
  const float dz = cz - (az + t * abz);
  return dx * dx + dz * dz <= radius * radius;
}

}  // namespace

CollisionBody collision_body_from_player(const PlayerBody& player) {
  CollisionBody body;
  body.x = player.x;
  body.y = player.y;
  body.z = player.z;
  body.radius = player.half_extent;
  body.height = kPlayerCylinderHeight;
  body.mass = 1.0f;
  body.vel_x = 0.0f;
  body.vel_y = 0.0f;
  body.vel_z = 0.0f;
  return body;
}

CollisionWorld bake_fence_world(std::span<const EdgeBarrierDef> barriers, const SurfaceQuery& query) {
  CollisionWorld world;
  const float ts = query.tile_size() > 0.0f ? query.tile_size() : 1.0f;
  world.fences.reserve(barriers.size());

  for (const EdgeBarrierDef& edge : barriers) {
    if (edge.height <= 0.0f) {
      continue;
    }

    const float ox = static_cast<float>(edge.tile.x) * ts;
    const float oz = static_cast<float>(edge.tile.z) * ts;
    const float owner_top = query.sample(ox + 0.5f * ts, oz + 0.5f * ts).y;

    FenceSolid solid;
    solid.y_lo = owner_top;
    solid.y_hi = owner_top + edge.height;

    switch (edge.direction) {
      case RampDirection::East:
        solid.ax = ox + ts;
        solid.az = oz;
        solid.bx = ox + ts;
        solid.bz = oz + ts;
        break;
      case RampDirection::West:
        solid.ax = ox;
        solid.az = oz;
        solid.bx = ox;
        solid.bz = oz + ts;
        break;
      case RampDirection::South:
        solid.ax = ox;
        solid.az = oz + ts;
        solid.bx = ox + ts;
        solid.bz = oz + ts;
        break;
      case RampDirection::North:
        solid.ax = ox;
        solid.az = oz;
        solid.bx = ox + ts;
        solid.bz = oz;
        break;
    }

    world.fences.push_back(solid);
  }

  return world;
}

bool cylinder_hits_fences(const CollisionBody& body, const CollisionWorld& world) {
  const float body_top = body.y + body.height;
  for (const FenceSolid& solid : world.fences) {
    if (body.y + kFenceFeetClearanceEpsilon >= solid.y_hi) {
      continue;
    }
    if (!ranges_overlap(body.y, body_top, solid.y_lo, solid.y_hi)) {
      continue;
    }
    if (circle_hits_segment(body.x, body.z, body.radius, solid.ax, solid.az, solid.bx, solid.bz)) {
      return true;
    }
  }
  return false;
}

}  // namespace rat
