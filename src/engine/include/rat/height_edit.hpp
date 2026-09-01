#pragma once

#include "rat/map_data.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace rat {

struct HeightEditResult {
  bool ok = false;
  std::string error;
};

struct HeightGetResult {
  bool ok = false;
  float value = 0.0f;
  std::string error;
};

[[nodiscard]] HeightEditResult ensure_valid_height_grid(MapData& map);
[[nodiscard]] HeightEditResult upgrade_map_schema_for_elevation(MapData& map);

[[nodiscard]] HeightGetResult get_tile_ground_y(const HeightGrid& grid, int tile_x, int tile_z);
[[nodiscard]] HeightEditResult set_tile_ground_y(HeightGrid& grid, int tile_x, int tile_z,
                                                 float ground_y);
[[nodiscard]] HeightEditResult adjust_tile_ground_y(HeightGrid& grid, int tile_x, int tile_z,
                                                    float delta_y);

[[nodiscard]] int find_ramp_index_by_tile(const std::vector<RampDef>& ramps, TileCoord tile);
[[nodiscard]] HeightEditResult add_or_replace_ramp(std::vector<RampDef>& ramps,
                                                   HeightGrid& grid,
                                                   const RampDef& ramp);
[[nodiscard]] bool remove_ramp_by_tile(std::vector<RampDef>& ramps, TileCoord tile);

inline constexpr float kPlaceCubeDeltaY = 1.0f;
inline constexpr float kEdgeBarrierMiniHeight = 0.45f;
inline constexpr float kEdgeBarrierFullHeight = 1.6f;

[[nodiscard]] HeightEditResult set_map_tile_ground_y(MapData& map, int tile_x, int tile_z,
                                                     float ground_y);
[[nodiscard]] HeightEditResult adjust_map_tile_ground_y(MapData& map, int tile_x, int tile_z,
                                                        float delta_y);
[[nodiscard]] HeightEditResult place_map_tile_cube(MapData& map, int tile_x, int tile_z);
[[nodiscard]] HeightEditResult upsert_map_ramp(MapData& map, const RampDef& ramp);
[[nodiscard]] HeightEditResult remove_map_ramp(MapData& map, TileCoord tile);
[[nodiscard]] HeightEditResult upsert_map_edge_barrier(MapData& map, const EdgeBarrierDef& edge);
[[nodiscard]] HeightEditResult remove_map_edge_barrier(MapData& map, TileCoord tile,
                                                       RampDirection direction);
[[nodiscard]] HeightEditResult upsert_map_floor_slab(MapData& map, FloorSlabDef slab);
[[nodiscard]] HeightEditResult upsert_map_ladder(MapData& map, LadderDef ladder);
[[nodiscard]] HeightEditResult remove_map_ladder(MapData& map, TileCoord tile,
                                                 RampDirection direction);

}  // namespace rat
