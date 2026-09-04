#include "rat/indoor_volume.hpp"

#include "rat/collision.hpp"
#include "rat/terrain_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rat {
namespace {

constexpr std::uint32_t kTerrainFillAbgr = 0xff707070;
constexpr std::uint32_t kFenceFillAbgr = 0xff8a5a38;
constexpr std::uint32_t kSlabFillAbgr = 0xff48a0c8;
constexpr std::uint32_t kLadderFillAbgr = 0xff38d070;

[[nodiscard]] bool circle_overlaps_aabb(float x, float z, float radius, const Aabb2& box) {
  const float closest_x = std::clamp(x, box.min_x, box.max_x);
  const float closest_z = std::clamp(z, box.min_z, box.max_z);
  const float dx = x - closest_x;
  const float dz = z - closest_z;
  return dx * dx + dz * dz <= radius * radius;
}

[[nodiscard]] bool cylinder_overlaps_volume(const PlayerBody& body, const IndoorVolume& volume) {
  if (!circle_overlaps_aabb(body.x, body.z, body.half_extent, volume.xz)) {
    return false;
  }
  const float head_y = body.y + kPlayerCylinderHeight;
  return body.y < volume.y_hi && head_y > volume.y_lo;
}

[[nodiscard]] bool point_in_volume(const IndoorVolume& volume, float x, float y, float z) {
  return x >= volume.xz.min_x && x <= volume.xz.max_x && z >= volume.xz.min_z &&
         z <= volume.xz.max_z && y >= volume.y_lo && y <= volume.y_hi;
}

void push_fill_quad(GreyboxFillMesh& mesh, float x0, float y0, float z0, float x1, float y1,
                    float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                    std::uint32_t color) {
  const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
  mesh.vertices.push_back(GreyboxFillVertex{x0, y0, z0, color});
  mesh.vertices.push_back(GreyboxFillVertex{x1, y1, z1, color});
  mesh.vertices.push_back(GreyboxFillVertex{x2, y2, z2, color});
  mesh.vertices.push_back(GreyboxFillVertex{x3, y3, z3, color});
  mesh.indices.push_back(static_cast<std::uint16_t>(base + 0));
  mesh.indices.push_back(static_cast<std::uint16_t>(base + 2));
  mesh.indices.push_back(static_cast<std::uint16_t>(base + 1));
  mesh.indices.push_back(static_cast<std::uint16_t>(base + 0));
  mesh.indices.push_back(static_cast<std::uint16_t>(base + 3));
  mesh.indices.push_back(static_cast<std::uint16_t>(base + 2));
}

}  // namespace

bool player_inside_indoor_volume(const MapData& map, const PlayerBody& body) {
  for (const IndoorVolume& volume : map.indoor_volumes) {
    if (cylinder_overlaps_volume(body, volume)) {
      return true;
    }
  }
  return false;
}

bool point_inside_indoor_volume(const MapData& map, float x, float y, float z) {
  for (const IndoorVolume& volume : map.indoor_volumes) {
    if (point_in_volume(volume, x, y, z)) {
      return true;
    }
  }
  return false;
}

std::uint32_t multiply_abgr(std::uint32_t abgr, float factor) {
  const float f = std::clamp(factor, 0.0f, 1.0f);
  const auto mul = [f](std::uint32_t channel) {
    return static_cast<std::uint32_t>(std::lround(static_cast<float>(channel) * f));
  };
  const std::uint32_t a = (abgr >> 24) & 0xffu;
  const std::uint32_t b = (abgr >> 16) & 0xffu;
  const std::uint32_t g = (abgr >> 8) & 0xffu;
  const std::uint32_t r = abgr & 0xffu;
  return (a << 24) | (mul(b) << 16) | (mul(g) << 8) | mul(r);
}

std::uint32_t greybox_fill_abgr(const MapData& map, const PlayerBody& body, float cx, float cy,
                                float cz, std::uint32_t base_abgr) {
  if (!player_inside_indoor_volume(map, body)) {
    return base_abgr;
  }
  if (point_inside_indoor_volume(map, cx, cy, cz)) {
    return base_abgr;
  }
  return multiply_abgr(base_abgr, kGreyboxIndoorDimFactor);
}

GreyboxFillMesh build_greybox_fill_mesh(const MapData& map, const PlayerBody& body,
                                        bool apply_indoor_dim) {
  GreyboxFillMesh mesh;
  if (choose_terrain_render_policy(map) != TerrainRenderPolicy::HeightTerrain) {
    return mesh;
  }
  const TerrainGeometry geometry = build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  if (geometry.tiles.empty()) {
    return mesh;
  }
  const std::vector<TerrainSideFace> side_faces = build_terrain_side_faces(geometry);
  const std::vector<TerrainSideFace> fence_faces =
      build_edge_barrier_faces(geometry, map.edge_barriers);
  constexpr std::size_t kLadderQuadsPerLadder = 6;
  std::size_t occupancy_solid_count = 0;
  for (const OccupancyCell& cell : map.occupancy) {
    if (cell.kind == OccupancyKind::Solid) {
      ++occupancy_solid_count;
    }
  }
  const std::size_t slab_quads = map.floor_slabs.size() * kFloorSlabFillQuadCount;
  const std::size_t ladder_quads = map.ladders.size() * kLadderQuadsPerLadder;
  const std::size_t occupancy_quads = occupancy_solid_count * kOccupancySolidFillQuadCount;
  if (!terrain_fill_quad_count_fits_u16(geometry.tiles.size(), side_faces.size(),
                                        fence_faces.size() + slab_quads + ladder_quads +
                                            occupancy_quads)) {
    return mesh;
  }

  const auto color_for = [&](float x0, float y0, float z0, float x1, float y1, float z1, float x2,
                             float y2, float z2, float x3, float y3, float z3,
                             std::uint32_t base) {
    if (!apply_indoor_dim) {
      return base;
    }
    const float cx = 0.25f * (x0 + x1 + x2 + x3);
    const float cy = 0.25f * (y0 + y1 + y2 + y3);
    const float cz = 0.25f * (z0 + z1 + z2 + z3);
    return greybox_fill_abgr(map, body, cx, cy, cz, base);
  };

  const std::size_t quad_count = geometry.tiles.size() + side_faces.size() + fence_faces.size() +
                                 slab_quads + ladder_quads + occupancy_quads;
  mesh.vertices.reserve(quad_count * 4);
  mesh.indices.reserve(quad_count * 6);

  auto push = [&](float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2,
                  float z2, float x3, float y3, float z3, std::uint32_t base) {
    push_fill_quad(mesh, x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3,
                   color_for(x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, base));
  };

  for (const TerrainTileQuad& tile : geometry.tiles) {
    push(tile.min_x, tile.y_nw, tile.min_z, tile.max_x, tile.y_ne, tile.min_z, tile.max_x,
         tile.y_se, tile.max_z, tile.min_x, tile.y_sw, tile.max_z, kTerrainFillAbgr);
  }
  for (const TerrainSideFace& face : side_faces) {
    push(face.x0, face.y0_lo, face.z0, face.x0, face.y0_hi, face.z0, face.x1, face.y1_hi, face.z1,
         face.x1, face.y1_lo, face.z1, kTerrainFillAbgr);
  }
  for (const TerrainSideFace& face : fence_faces) {
    push(face.x0, face.y0_lo, face.z0, face.x0, face.y0_hi, face.z0, face.x1, face.y1_hi, face.z1,
         face.x1, face.y1_lo, face.z1, kFenceFillAbgr);
  }
  const float ts = map.tile_size > 0.0f ? map.tile_size : 1.0f;
  for (const FloorSlabDef& slab : map.floor_slabs) {
    const std::vector<TerrainFillQuad> fill = build_floor_slab_fill_quads(slab, ts);
    for (const TerrainFillQuad& quad : fill) {
      push(quad.x0, quad.y0, quad.z0, quad.x1, quad.y1, quad.z1, quad.x2, quad.y2, quad.z2, quad.x3,
           quad.y3, quad.z3, kSlabFillAbgr);
    }
  }
  for (const LadderDef& ladder : map.ladders) {
    const float ox = static_cast<float>(ladder.tile.x) * ts;
    const float oz = static_cast<float>(ladder.tile.z) * ts;
    float min_x = ox;
    float max_x = ox + ts;
    float min_z = oz;
    float max_z = oz + ts;
    switch (ladder.direction) {
      case RampDirection::East:
        min_x = ox + ts - kLadderInset;
        max_x = ox + ts;
        break;
      case RampDirection::West:
        min_x = ox;
        max_x = ox + kLadderInset;
        break;
      case RampDirection::South:
        min_z = oz + ts - kLadderInset;
        max_z = oz + ts;
        break;
      case RampDirection::North:
        min_z = oz;
        max_z = oz + kLadderInset;
        break;
    }
    const float y_hi = ladder.y_hi;
    const float y_lo = ladder.y_lo;
    push(min_x, y_hi, min_z, max_x, y_hi, min_z, max_x, y_hi, max_z, min_x, y_hi, max_z,
         kLadderFillAbgr);
    push(min_x, y_lo, max_z, max_x, y_lo, max_z, max_x, y_lo, min_z, min_x, y_lo, min_z,
         kLadderFillAbgr);
    push(min_x, y_lo, min_z, min_x, y_hi, min_z, max_x, y_hi, min_z, max_x, y_lo, min_z,
         kLadderFillAbgr);
    push(max_x, y_lo, min_z, max_x, y_hi, min_z, max_x, y_hi, max_z, max_x, y_lo, max_z,
         kLadderFillAbgr);
    push(max_x, y_lo, max_z, max_x, y_hi, max_z, min_x, y_hi, max_z, min_x, y_lo, max_z,
         kLadderFillAbgr);
    push(min_x, y_lo, max_z, min_x, y_hi, max_z, min_x, y_hi, min_z, min_x, y_lo, min_z,
         kLadderFillAbgr);
  }
  for (const OccupancyCell& cell : map.occupancy) {
    if (cell.kind != OccupancyKind::Solid) {
      continue;
    }
    const std::vector<TerrainFillQuad> fill = build_occupancy_solid_fill_quads(cell, ts);
    for (const TerrainFillQuad& quad : fill) {
      push(quad.x0, quad.y0, quad.z0, quad.x1, quad.y1, quad.z1, quad.x2, quad.y2, quad.z2, quad.x3,
           quad.y3, quad.z3, kTerrainFillAbgr);
    }
  }
  return mesh;
}

}  // namespace rat
