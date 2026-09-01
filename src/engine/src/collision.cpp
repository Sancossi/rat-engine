#include "rat/collision.hpp"
#include "rat/terrain_geometry.hpp"

#include <algorithm>

namespace rat {
namespace {

bool ranges_overlap(float min_a, float max_a, float min_b, float max_b) {
  return min_a < max_b && max_a > min_b;
}

bool circle_hits_segment(float cx, float cz, float radius, float ax, float az, float bx, float bz,
                         float& t_out) {
  const float abx = bx - ax;
  const float abz = bz - az;
  const float acx = cx - ax;
  const float acz = cz - az;
  const float ab_len2 = abx * abx + abz * abz;
  float t = 0.0f;
  if (ab_len2 > 0.0f) {
    t = std::clamp((acx * abx + acz * abz) / ab_len2, 0.0f, 1.0f);
  }
  t_out = t;
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
    solid.ay_lo = solid.y_lo;
    solid.ay_hi = solid.y_hi;
    solid.by_lo = solid.y_lo;
    solid.by_hi = solid.y_hi;

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

void append_terrain_walls(CollisionWorld& world, const HeightGrid& grid,
                          std::span<const RampDef> ramps, float tile_size) {
  const TerrainGeometry geometry = build_terrain_geometry(grid, ramps, tile_size);
  const std::vector<TerrainSideFace> faces = build_terrain_side_faces(geometry);
  world.fences.reserve(world.fences.size() + faces.size());
  for (const TerrainSideFace& face : faces) {
    FenceSolid solid;
    solid.ax = face.x0;
    solid.az = face.z0;
    solid.bx = face.x1;
    solid.bz = face.z1;
    solid.ay_lo = face.y0_lo;
    solid.ay_hi = face.y0_hi;
    solid.by_lo = face.y1_lo;
    solid.by_hi = face.y1_hi;
    solid.y_lo = std::min(solid.ay_lo, solid.by_lo);
    solid.y_hi = std::max(solid.ay_hi, solid.by_hi);
    if (solid.y_lo == solid.y_hi) {
      continue;
    }
    world.fences.push_back(solid);
  }
}

CollisionWorld bake_collision_world(const MapData& map, const SurfaceQuery& query) {
  CollisionWorld world = bake_fence_world(map.edge_barriers, query);
  const float ts = query.tile_size() > 0.0f ? query.tile_size() : 1.0f;
  append_terrain_walls(world, map.height_grid, map.ramps, ts);
  return world;
}

bool cylinder_hits_walls(const CollisionBody& body, const CollisionWorld& world, float max_step_up) {
  const float body_top = body.y + body.height;
  for (const FenceSolid& solid : world.fences) {
    if (body.y + kFenceFeetClearanceEpsilon >= solid.y_hi) {
      continue;
    }
    float t = 0.0f;
    if (!circle_hits_segment(body.x, body.z, body.radius, solid.ax, solid.az, solid.bx, solid.bz,
                             t)) {
      continue;
    }
    const float y_lo = solid.ay_lo + (solid.by_lo - solid.ay_lo) * t;
    const float y_hi = solid.ay_hi + (solid.by_hi - solid.ay_hi) * t;
    if (body.y + kFenceFeetClearanceEpsilon >= y_hi) {
      continue;
    }
    if ((y_hi - y_lo) <= max_step_up) {
      continue;
    }
    if ((y_hi - body.y) <= max_step_up) {
      continue;
    }
    if (!ranges_overlap(body.y, body_top, y_lo, y_hi)) {
      continue;
    }
    return true;
  }
  return false;
}

bool cylinder_hits_fences(const CollisionBody& body, const CollisionWorld& world) {
  return cylinder_hits_walls(body, world, 0.0f);
}

bool circle_overlaps_aabb2(float cx, float cz, float radius, const Aabb2& box) {
  const float closest_x = std::clamp(cx, box.min_x, box.max_x);
  const float closest_z = std::clamp(cz, box.min_z, box.max_z);
  const float dx = cx - closest_x;
  const float dz = cz - closest_z;
  return dx * dx + dz * dz <= radius * radius;
}

}  // namespace rat
