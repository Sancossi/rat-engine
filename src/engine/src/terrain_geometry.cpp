#include "rat/terrain_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace rat {
namespace {

float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

float clamp(float value, float lo, float hi) {
  return std::max(lo, std::min(value, hi));
}

void apply_ramp_to_corners(TerrainTileQuad& tile, RampDirection direction, float low_y,
                           float high_y) {
  switch (direction) {
    case RampDirection::North:
      tile.y_nw = high_y;
      tile.y_ne = high_y;
      tile.y_se = low_y;
      tile.y_sw = low_y;
      break;
    case RampDirection::East:
      tile.y_nw = low_y;
      tile.y_ne = high_y;
      tile.y_se = high_y;
      tile.y_sw = low_y;
      break;
    case RampDirection::South:
      tile.y_nw = low_y;
      tile.y_ne = low_y;
      tile.y_se = high_y;
      tile.y_sw = high_y;
      break;
    case RampDirection::West:
      tile.y_nw = high_y;
      tile.y_ne = low_y;
      tile.y_se = low_y;
      tile.y_sw = high_y;
      break;
  }
}

}  // namespace

TerrainGeometry build_terrain_geometry(const HeightGrid& grid, std::span<const RampDef> ramps,
                                       float tile_size) {
  TerrainGeometry out;
  out.origin_x = grid.origin_x;
  out.origin_z = grid.origin_z;
  out.width = std::max(0, grid.width);
  out.height = std::max(0, grid.height);
  out.tile_size = tile_size > 0.0f ? tile_size : 1.0f;

  if (out.width <= 0 || out.height <= 0) {
    return out;
  }
  const std::size_t expected = static_cast<std::size_t>(out.width) * out.height;
  if (grid.ground_y.size() != expected) {
    return out;
  }

  out.tiles.reserve(expected);
  for (int z = 0; z < out.height; ++z) {
    for (int x = 0; x < out.width; ++x) {
      const std::size_t index = static_cast<std::size_t>(z) * out.width + x;
      const float base_y = grid.ground_y[index];
      TerrainTileQuad tile;
      tile.min_x = static_cast<float>(out.origin_x + x) * out.tile_size;
      tile.min_z = static_cast<float>(out.origin_z + z) * out.tile_size;
      tile.max_x = tile.min_x + out.tile_size;
      tile.max_z = tile.min_z + out.tile_size;
      tile.y_nw = base_y;
      tile.y_ne = base_y;
      tile.y_se = base_y;
      tile.y_sw = base_y;

      const int world_tile_x = out.origin_x + x;
      const int world_tile_z = out.origin_z + z;
      for (const RampDef& ramp : ramps) {
        if (ramp.tile.x != world_tile_x || ramp.tile.z != world_tile_z) {
          continue;
        }
        tile.on_ramp = true;
        tile.ramp_direction = ramp.direction;
        tile.ramp_low_y = ramp.low_y;
        tile.ramp_high_y = ramp.high_y;
        apply_ramp_to_corners(tile, ramp.direction, ramp.low_y, ramp.high_y);
        break;
      }

      out.tiles.push_back(tile);
    }
  }

  return out;
}

float sample_terrain_height(const TerrainGeometry& geometry, float world_x, float world_z) {
  if (geometry.width <= 0 || geometry.height <= 0 || geometry.tile_size <= 0.0f) {
    return 0.0f;
  }
  const float tile_size = geometry.tile_size;
  const float local_x = world_x / tile_size - static_cast<float>(geometry.origin_x);
  const float local_z = world_z / tile_size - static_cast<float>(geometry.origin_z);
  const int tile_x = static_cast<int>(std::floor(local_x));
  const int tile_z = static_cast<int>(std::floor(local_z));
  if (tile_x < 0 || tile_z < 0 || tile_x >= geometry.width || tile_z >= geometry.height) {
    return 0.0f;
  }

  const std::size_t index = static_cast<std::size_t>(tile_z) * geometry.width + tile_x;
  if (index >= geometry.tiles.size()) {
    return 0.0f;
  }
  const TerrainTileQuad& tile = geometry.tiles[index];
  if (!tile.on_ramp) {
    return tile.y_nw;
  }

  const float frac_x = clamp01((world_x - tile.min_x) / tile_size);
  const float frac_z = clamp01((world_z - tile.min_z) / tile_size);
  float t = 0.0f;
  switch (tile.ramp_direction) {
    case RampDirection::North:
      t = 1.0f - frac_z;
      break;
    case RampDirection::East:
      t = frac_x;
      break;
    case RampDirection::South:
      t = frac_z;
      break;
    case RampDirection::West:
      t = 1.0f - frac_x;
      break;
  }
  return tile.ramp_low_y + (tile.ramp_high_y - tile.ramp_low_y) * clamp01(t);
}

float sample_terrain_height_clamped(const TerrainGeometry& geometry, float world_x, float world_z) {
  if (geometry.width <= 0 || geometry.height <= 0 || geometry.tile_size <= 0.0f) {
    return 0.0f;
  }
  const float min_x = static_cast<float>(geometry.origin_x) * geometry.tile_size;
  const float min_z = static_cast<float>(geometry.origin_z) * geometry.tile_size;
  const float max_x = min_x + static_cast<float>(geometry.width) * geometry.tile_size;
  const float max_z = min_z + static_cast<float>(geometry.height) * geometry.tile_size;
  const float eps = std::max(1e-4f, geometry.tile_size * 1e-4f);
  const float x = clamp(world_x, min_x + eps, max_x - eps);
  const float z = clamp(world_z, min_z + eps, max_z - eps);
  return sample_terrain_height(geometry, x, z);
}

std::vector<TerrainLineSegment> build_terrain_grid_lines(const TerrainGeometry& geometry,
                                                         float line_offset) {
  std::vector<TerrainLineSegment> out;
  if (geometry.width <= 0 || geometry.height <= 0 || geometry.tile_size <= 0.0f ||
      geometry.tiles.empty()) {
    return out;
  }
  const float min_x = static_cast<float>(geometry.origin_x) * geometry.tile_size;
  const float min_z = static_cast<float>(geometry.origin_z) * geometry.tile_size;
  const std::size_t horizontal =
      static_cast<std::size_t>(geometry.height + 1) * static_cast<std::size_t>(geometry.width);
  const std::size_t vertical =
      static_cast<std::size_t>(geometry.width + 1) * static_cast<std::size_t>(geometry.height);
  out.reserve(horizontal + vertical);

  for (int z = 0; z <= geometry.height; ++z) {
    const float world_z = min_z + static_cast<float>(z) * geometry.tile_size;
    for (int x = 0; x < geometry.width; ++x) {
      const float x0 = min_x + static_cast<float>(x) * geometry.tile_size;
      const float x1 = x0 + geometry.tile_size;
      const float y0 = sample_terrain_height_clamped(geometry, x0, world_z) + line_offset;
      const float y1 = sample_terrain_height_clamped(geometry, x1, world_z) + line_offset;
      out.push_back(TerrainLineSegment{
          TerrainLineVertex{x0, y0, world_z},
          TerrainLineVertex{x1, y1, world_z},
      });
    }
  }

  for (int x = 0; x <= geometry.width; ++x) {
    const float world_x = min_x + static_cast<float>(x) * geometry.tile_size;
    for (int z = 0; z < geometry.height; ++z) {
      const float z0 = min_z + static_cast<float>(z) * geometry.tile_size;
      const float z1 = z0 + geometry.tile_size;
      const float y0 = sample_terrain_height_clamped(geometry, world_x, z0) + line_offset;
      const float y1 = sample_terrain_height_clamped(geometry, world_x, z1) + line_offset;
      out.push_back(TerrainLineSegment{
          TerrainLineVertex{world_x, y0, z0},
          TerrainLineVertex{world_x, y1, z1},
      });
    }
  }
  return out;
}

TerrainRenderPolicy choose_terrain_render_policy(const MapData& map) {
  if (map.schema_version < 2) {
    return TerrainRenderPolicy::LegacyFlat;
  }
  const HeightGrid& grid = map.height_grid;
  const int width = std::max(0, grid.width);
  const int height = std::max(0, grid.height);
  if (width <= 0 || height <= 0) {
    return TerrainRenderPolicy::LegacyFlat;
  }
  const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  if (grid.ground_y.size() != expected) {
    return TerrainRenderPolicy::LegacyFlat;
  }
  return TerrainRenderPolicy::HeightTerrain;
}

bool terrain_fill_quad_count_fits_u16(std::size_t tile_count, std::size_t side_face_count) {
  // 4 vertices per fill quad (top or side), indices addressed by uint16.
  constexpr std::size_t kMaxQuadCount = static_cast<std::size_t>(65535u / 4u);
  if (tile_count > kMaxQuadCount) {
    return false;
  }
  return side_face_count <= kMaxQuadCount - tile_count;
}

bool terrain_tile_count_fits_u16(std::size_t tile_count) {
  return terrain_fill_quad_count_fits_u16(tile_count, 0);
}

std::vector<TerrainSideFace> build_terrain_side_faces(const TerrainGeometry& geometry) {
  std::vector<TerrainSideFace> out;
  if (geometry.width <= 0 || geometry.height <= 0 || geometry.tiles.empty()) {
    return out;
  }
  const std::size_t expected =
      static_cast<std::size_t>(geometry.width) * static_cast<std::size_t>(geometry.height);
  if (geometry.tiles.size() != expected) {
    return out;
  }

  auto tile_at = [&](int x, int z) -> const TerrainTileQuad& {
    return geometry.tiles[static_cast<std::size_t>(z) * static_cast<std::size_t>(geometry.width) +
                          static_cast<std::size_t>(x)];
  };

  auto maybe_push = [&](float x0, float z0, float y0_a, float y0_b, float x1, float z1, float y1_a,
                        float y1_b) {
    if (y0_a == y0_b && y1_a == y1_b) {
      return;
    }
    TerrainSideFace face;
    face.x0 = x0;
    face.z0 = z0;
    face.y0_lo = std::min(y0_a, y0_b);
    face.y0_hi = std::max(y0_a, y0_b);
    face.x1 = x1;
    face.z1 = z1;
    face.y1_lo = std::min(y1_a, y1_b);
    face.y1_hi = std::max(y1_a, y1_b);
    out.push_back(face);
  };

  for (int z = 0; z < geometry.height; ++z) {
    for (int x = 0; x < geometry.width; ++x) {
      const TerrainTileQuad& tile = tile_at(x, z);
      if (x + 1 < geometry.width) {
        const TerrainTileQuad& east = tile_at(x + 1, z);
        maybe_push(tile.max_x, tile.min_z, tile.y_ne, east.y_nw, tile.max_x, tile.max_z, tile.y_se,
                   east.y_sw);
      }
      if (z + 1 < geometry.height) {
        const TerrainTileQuad& south = tile_at(x, z + 1);
        maybe_push(tile.min_x, tile.max_z, tile.y_sw, south.y_nw, tile.max_x, tile.max_z, tile.y_se,
                   south.y_ne);
      }
    }
  }
  return out;
}

}  // namespace rat
