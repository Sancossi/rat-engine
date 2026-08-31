#pragma once

#include "rat/map_data.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace rat {

struct TerrainTileQuad {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
  float y_nw = 0.0f;
  float y_ne = 0.0f;
  float y_se = 0.0f;
  float y_sw = 0.0f;
  bool on_ramp = false;
  RampDirection ramp_direction = RampDirection::North;
  float ramp_low_y = 0.0f;
  float ramp_high_y = 0.0f;
};

struct TerrainGeometry {
  int origin_x = 0;
  int origin_z = 0;
  int width = 0;
  int height = 0;
  float tile_size = 1.0f;
  std::vector<TerrainTileQuad> tiles;
};

enum class TerrainRenderPolicy {
  LegacyFlat,
  HeightTerrain,
};

struct TerrainLineVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct TerrainLineSegment {
  TerrainLineVertex a{};
  TerrainLineVertex b{};
};

[[nodiscard]] TerrainGeometry build_terrain_geometry(const HeightGrid& grid,
                                                     std::span<const RampDef> ramps,
                                                     float tile_size);

[[nodiscard]] float sample_terrain_height(const TerrainGeometry& geometry, float world_x,
                                          float world_z);
[[nodiscard]] float sample_terrain_height_clamped(const TerrainGeometry& geometry, float world_x,
                                                  float world_z);
[[nodiscard]] std::vector<TerrainLineSegment> build_terrain_grid_lines(
    const TerrainGeometry& geometry, float line_offset = 0.03f);
[[nodiscard]] TerrainRenderPolicy choose_terrain_render_policy(const MapData& map);
[[nodiscard]] bool terrain_tile_count_fits_u16(std::size_t tile_count);

}  // namespace rat
