#include "rat/collision.hpp"
#include "rat/terrain_geometry.hpp"

#include <algorithm>
#include <cmath>

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

bool tile_has_ramp(std::span<const RampDef> ramps, int world_x, int world_z) {
  for (const RampDef& ramp : ramps) {
    if (ramp.tile.x == world_x && ramp.tile.z == world_z) {
      return true;
    }
  }
  return false;
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

void append_ground_boxes(CollisionWorld& world, const HeightGrid& grid,
                         std::span<const RampDef> ramps, float tile_size) {
  const int width = std::max(0, grid.width);
  const int height = std::max(0, grid.height);
  const float ts = tile_size > 0.0f ? tile_size : 1.0f;
  if (width <= 0 || height <= 0) {
    return;
  }
  const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  if (grid.ground_y.size() != expected) {
    return;
  }

  world.boxes.reserve(world.boxes.size() + expected);
  for (int z = 0; z < height; ++z) {
    for (int x = 0; x < width; ++x) {
      const int world_x = grid.origin_x + x;
      const int world_z = grid.origin_z + z;
      if (tile_has_ramp(ramps, world_x, world_z)) {
        continue;
      }
      const std::size_t index = static_cast<std::size_t>(z) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x);
      WalkableBox box;
      box.min_x = static_cast<float>(world_x) * ts;
      box.min_z = static_cast<float>(world_z) * ts;
      box.max_x = box.min_x + ts;
      box.max_z = box.min_z + ts;
      box.y_lo = 0.0f;
      box.y_hi = grid.ground_y[index];
      world.boxes.push_back(box);
    }
  }
}

void append_floor_slabs(CollisionWorld& world, std::span<const FloorSlabDef> slabs, float tile_size) {
  const float ts = tile_size > 0.0f ? tile_size : 1.0f;
  world.boxes.reserve(world.boxes.size() + slabs.size());
  for (const FloorSlabDef& slab : slabs) {
    WalkableBox box;
    box.min_x = static_cast<float>(slab.tile.x) * ts;
    box.min_z = static_cast<float>(slab.tile.z) * ts;
    box.max_x = box.min_x + ts;
    box.max_z = box.min_z + ts;
    box.y_hi = slab.top_y;
    box.y_lo = slab.top_y - slab.thickness;
    world.boxes.push_back(box);
  }
}

CollisionWorld bake_collision_world(const MapData& map, const SurfaceQuery& query) {
  CollisionWorld world = bake_fence_world(map.edge_barriers, query);
  const float ts = query.tile_size() > 0.0f ? query.tile_size() : 1.0f;
  append_terrain_walls(world, map.height_grid, map.ramps, ts);
  append_ground_boxes(world, map.height_grid, map.ramps, ts);
  append_floor_slabs(world, map.floor_slabs, ts);
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
    if (!ranges_overlap(body.y, body_top, y_lo, y_hi)) {
      continue;
    }
    return true;
  }
  return false;
}

void depenetrate_cylinder_from_walls(CollisionBody& body, const CollisionWorld& world,
                                     float max_step_up) {
  constexpr int kMaxIters = 4;
  const float body_top = body.y + body.height;
  for (int iter = 0; iter < kMaxIters; ++iter) {
    if (!cylinder_hits_walls(body, world, max_step_up)) {
      return;
    }
    bool pushed = false;
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
      if (!ranges_overlap(body.y, body_top, y_lo, y_hi)) {
        continue;
      }
      const float px = solid.ax + (solid.bx - solid.ax) * t;
      const float pz = solid.az + (solid.bz - solid.az) * t;
      float dx = body.x - px;
      float dz = body.z - pz;
      float dist = std::sqrt(dx * dx + dz * dz);
      const float needed = body.radius + kFenceFeetClearanceEpsilon;
      if (dist > 1e-8f) {
        const float scale = needed / dist;
        body.x = px + dx * scale;
        body.z = pz + dz * scale;
      } else {
        dx = -(solid.bz - solid.az);
        dz = solid.bx - solid.ax;
        dist = std::sqrt(dx * dx + dz * dz);
        if (dist <= 1e-8f) {
          continue;
        }
        const float scale = needed / dist;
        CollisionBody cand = body;
        cand.x = px + dx * scale;
        cand.z = pz + dz * scale;
        if (cylinder_hits_walls(cand, world, max_step_up)) {
          cand.x = px - dx * scale;
          cand.z = pz - dz * scale;
        }
        body.x = cand.x;
        body.z = cand.z;
      }
      pushed = true;
      break;
    }
    if (!pushed) {
      return;
    }
  }
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
