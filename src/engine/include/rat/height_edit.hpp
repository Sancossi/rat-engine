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

[[nodiscard]] HeightEditResult set_map_tile_ground_y(MapData& map, int tile_x, int tile_z,
                                                     float ground_y);
[[nodiscard]] HeightEditResult adjust_map_tile_ground_y(MapData& map, int tile_x, int tile_z,
                                                        float delta_y);
[[nodiscard]] HeightEditResult upsert_map_ramp(MapData& map, const RampDef& ramp);
[[nodiscard]] HeightEditResult remove_map_ramp(MapData& map, TileCoord tile);

}  // namespace rat
