#include "rat/height_edit.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace rat {
namespace {

HeightEditResult ok_result() {
  return HeightEditResult{true, {}};
}

HeightEditResult error_result(std::string message) {
  return HeightEditResult{false, std::move(message)};
}

bool is_valid_direction(RampDirection direction) {
  switch (direction) {
    case RampDirection::North:
    case RampDirection::East:
    case RampDirection::South:
    case RampDirection::West:
      return true;
  }
  return false;
}

bool grid_dimensions_valid(const HeightGrid& grid) {
  return grid.width > 0 && grid.height > 0;
}

bool has_ramp_on_tile(const std::vector<RampDef>& ramps, int tile_x, int tile_z) {
  for (const RampDef& ramp : ramps) {
    if (ramp.tile.x == tile_x && ramp.tile.z == tile_z) {
      return true;
    }
  }
  return false;
}

std::size_t expected_grid_cells(const HeightGrid& grid) {
  if (!grid_dimensions_valid(grid)) {
    return 0;
  }
  const std::size_t width = static_cast<std::size_t>(grid.width);
  const std::size_t height = static_cast<std::size_t>(grid.height);
  if (height != 0 && width > std::numeric_limits<std::size_t>::max() / height) {
    return 0;
  }
  return width * height;
}

HeightEditResult tile_to_index(const HeightGrid& grid, int tile_x, int tile_z, std::size_t& out) {
  if (!grid_dimensions_valid(grid)) {
    return error_result("height grid has invalid bounds");
  }
  const int local_x = tile_x - grid.origin_x;
  const int local_z = tile_z - grid.origin_z;
  if (local_x < 0 || local_z < 0 || local_x >= grid.width || local_z >= grid.height) {
    return error_result("tile is outside height grid bounds");
  }

  const std::size_t expected = expected_grid_cells(grid);
  if (expected == 0) {
    return error_result("height grid cell count overflow");
  }
  if (grid.ground_y.size() != expected) {
    return error_result("height grid values length mismatch");
  }

  out = static_cast<std::size_t>(local_z) * static_cast<std::size_t>(grid.width) +
        static_cast<std::size_t>(local_x);
  return ok_result();
}

HeightEditResult validate_ramp(const HeightGrid& grid, const RampDef& ramp) {
  if (!is_valid_direction(ramp.direction)) {
    return error_result("ramp direction is invalid");
  }
  if (ramp.high_y < ramp.low_y) {
    return error_result("ramp high_y must be >= low_y");
  }
  std::size_t ignored = 0;
  return tile_to_index(grid, ramp.tile.x, ramp.tile.z, ignored);
}

}  // namespace

HeightEditResult ensure_valid_height_grid(MapData& map) {
  if (map.width <= 0 || map.height <= 0) {
    return error_result("map width/height must be > 0");
  }

  HeightGrid source = map.height_grid;
  const int target_origin_x = source.origin_x;
  const int target_origin_z = source.origin_z;
  const int target_width = source.width > 0 ? source.width : map.width;
  const int target_height = source.height > 0 ? source.height : map.height;
  if (target_width <= 0 || target_height <= 0) {
    return error_result("height grid width/height must be > 0");
  }

  HeightGrid normalized;
  normalized.origin_x = target_origin_x;
  normalized.origin_z = target_origin_z;
  normalized.width = target_width;
  normalized.height = target_height;
  normalized.ground_y.assign(static_cast<std::size_t>(target_width) * static_cast<std::size_t>(target_height),
                             0.0f);

  const bool source_dims_ok = source.width > 0 && source.height > 0;
  const std::size_t source_expected =
      source_dims_ok ? static_cast<std::size_t>(source.width) * static_cast<std::size_t>(source.height) : 0;
  if (source_dims_ok && source_expected > 0 && source.ground_y.size() == source_expected) {
    for (int z = 0; z < source.height; ++z) {
      for (int x = 0; x < source.width; ++x) {
        const int world_x = source.origin_x + x;
        const int world_z = source.origin_z + z;
        const int nx = world_x - normalized.origin_x;
        const int nz = world_z - normalized.origin_z;
        if (nx < 0 || nz < 0 || nx >= normalized.width || nz >= normalized.height) {
          continue;
        }
        const std::size_t source_index =
            static_cast<std::size_t>(z) * static_cast<std::size_t>(source.width) + static_cast<std::size_t>(x);
        const std::size_t target_index = static_cast<std::size_t>(nz) *
                                             static_cast<std::size_t>(normalized.width) +
                                         static_cast<std::size_t>(nx);
        normalized.ground_y[target_index] = source.ground_y[source_index];
      }
    }
  }

  map.height_grid = std::move(normalized);

  map.ramps.erase(std::remove_if(map.ramps.begin(), map.ramps.end(),
                                 [&](const RampDef& ramp) {
                                   std::size_t ignored = 0;
                                   return !tile_to_index(map.height_grid, ramp.tile.x, ramp.tile.z, ignored).ok;
                                 }),
                  map.ramps.end());
  return ok_result();
}

HeightEditResult upgrade_map_schema_for_elevation(MapData& map) {
  if (map.schema_version < 2) {
    map.schema_version = 2;
    map.height_grid.origin_x = 0;
    map.height_grid.origin_z = 0;
    map.height_grid.width = map.width;
    map.height_grid.height = map.height;
  }
  return ensure_valid_height_grid(map);
}

HeightGetResult get_tile_ground_y(const HeightGrid& grid, int tile_x, int tile_z) {
  std::size_t index = 0;
  const HeightEditResult indexed = tile_to_index(grid, tile_x, tile_z, index);
  if (!indexed.ok) {
    return HeightGetResult{false, 0.0f, indexed.error};
  }
  return HeightGetResult{true, grid.ground_y[index], {}};
}

HeightEditResult set_tile_ground_y(HeightGrid& grid, int tile_x, int tile_z, float ground_y) {
  std::size_t index = 0;
  const HeightEditResult indexed = tile_to_index(grid, tile_x, tile_z, index);
  if (!indexed.ok) {
    return indexed;
  }
  grid.ground_y[index] = ground_y;
  return ok_result();
}

HeightEditResult adjust_tile_ground_y(HeightGrid& grid, int tile_x, int tile_z, float delta_y) {
  std::size_t index = 0;
  const HeightEditResult indexed = tile_to_index(grid, tile_x, tile_z, index);
  if (!indexed.ok) {
    return indexed;
  }
  grid.ground_y[index] += delta_y;
  return ok_result();
}

int find_ramp_index_by_tile(const std::vector<RampDef>& ramps, TileCoord tile) {
  for (std::size_t i = 0; i < ramps.size(); ++i) {
    if (ramps[i].tile.x == tile.x && ramps[i].tile.z == tile.z) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

HeightEditResult add_or_replace_ramp(std::vector<RampDef>& ramps, HeightGrid& grid,
                                     const RampDef& ramp) {
  const HeightEditResult checked = validate_ramp(grid, ramp);
  if (!checked.ok) {
    return checked;
  }

  const int first = find_ramp_index_by_tile(ramps, ramp.tile);
  if (first < 0) {
    ramps.push_back(ramp);
    return set_tile_ground_y(grid, ramp.tile.x, ramp.tile.z, ramp.low_y);
  }

  ramps[static_cast<std::size_t>(first)] = ramp;
  ramps.erase(
      std::remove_if(ramps.begin() + static_cast<std::size_t>(first) + 1, ramps.end(),
                     [&](const RampDef& existing) {
                       return existing.tile.x == ramp.tile.x && existing.tile.z == ramp.tile.z;
                     }),
      ramps.end());
  return set_tile_ground_y(grid, ramp.tile.x, ramp.tile.z, ramp.low_y);
}

bool remove_ramp_by_tile(std::vector<RampDef>& ramps, TileCoord tile) {
  const std::size_t before = ramps.size();
  ramps.erase(std::remove_if(ramps.begin(), ramps.end(),
                             [&](const RampDef& existing) {
                               return existing.tile.x == tile.x && existing.tile.z == tile.z;
                             }),
              ramps.end());
  return ramps.size() != before;
}

HeightEditResult set_map_tile_ground_y(MapData& map, int tile_x, int tile_z, float ground_y) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  if (has_ramp_on_tile(map.ramps, tile_x, tile_z)) {
    return error_result("cannot set ground_y on tile with ramp");
  }
  return set_tile_ground_y(map.height_grid, tile_x, tile_z, ground_y);
}

HeightEditResult adjust_map_tile_ground_y(MapData& map, int tile_x, int tile_z, float delta_y) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  if (has_ramp_on_tile(map.ramps, tile_x, tile_z)) {
    return error_result("cannot adjust ground_y on tile with ramp");
  }
  return adjust_tile_ground_y(map.height_grid, tile_x, tile_z, delta_y);
}

HeightEditResult place_map_tile_cube(MapData& map, int tile_x, int tile_z) {
  return adjust_map_tile_ground_y(map, tile_x, tile_z, kPlaceCubeDeltaY);
}

HeightEditResult upsert_map_ramp(MapData& map, const RampDef& ramp) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  const HeightEditResult edited = add_or_replace_ramp(map.ramps, map.height_grid, ramp);
  if (!edited.ok) {
    return edited;
  }
  map.edge_barriers.erase(
      std::remove_if(map.edge_barriers.begin(), map.edge_barriers.end(),
                     [&](const EdgeBarrierDef& existing) {
                       return existing.tile.x == ramp.tile.x && existing.tile.z == ramp.tile.z;
                     }),
      map.edge_barriers.end());
  return edited;
}

HeightEditResult remove_map_ramp(MapData& map, TileCoord tile) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  if (!remove_ramp_by_tile(map.ramps, tile)) {
    return error_result("ramp not found for tile");
  }
  return ok_result();
}

HeightEditResult upsert_map_edge_barrier(MapData& map, const EdgeBarrierDef& edge) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  if (!is_valid_direction(edge.direction)) {
    return error_result("edge barrier direction is invalid");
  }
  if (edge.height <= 0.0f) {
    return error_result("edge barrier height must be > 0");
  }
  std::size_t ignored = 0;
  const HeightEditResult indexed =
      tile_to_index(map.height_grid, edge.tile.x, edge.tile.z, ignored);
  if (!indexed.ok) {
    return indexed;
  }
  if (has_ramp_on_tile(map.ramps, edge.tile.x, edge.tile.z)) {
    return error_result("cannot place edge barrier on tile with ramp");
  }
  for (EdgeBarrierDef& existing : map.edge_barriers) {
    if (existing.tile.x == edge.tile.x && existing.tile.z == edge.tile.z &&
        existing.direction == edge.direction) {
      existing = edge;
      return ok_result();
    }
  }
  map.edge_barriers.push_back(edge);
  return ok_result();
}

HeightEditResult remove_map_edge_barrier(MapData& map, TileCoord tile, RampDirection direction) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  const std::size_t before = map.edge_barriers.size();
  map.edge_barriers.erase(std::remove_if(map.edge_barriers.begin(), map.edge_barriers.end(),
                                         [&](const EdgeBarrierDef& existing) {
                                           return existing.tile.x == tile.x &&
                                                  existing.tile.z == tile.z &&
                                                  existing.direction == direction;
                                         }),
                          map.edge_barriers.end());
  if (map.edge_barriers.size() == before) {
    return error_result("edge barrier not found for tile and direction");
  }
  return ok_result();
}

HeightEditResult upsert_map_floor_slab(MapData& map, FloorSlabDef slab) {
  const HeightEditResult upgraded = upgrade_map_schema_for_elevation(map);
  if (!upgraded.ok) {
    return upgraded;
  }
  if (slab.thickness <= 0.0f) {
    return error_result("floor slab thickness must be > 0");
  }
  std::size_t ignored = 0;
  const HeightEditResult indexed =
      tile_to_index(map.height_grid, slab.tile.x, slab.tile.z, ignored);
  if (!indexed.ok) {
    return indexed;
  }
  if (map.schema_version < 3) {
    map.schema_version = 3;
  }
  constexpr float kSameTopY = 1e-4f;
  for (auto it = map.floor_slabs.begin(); it != map.floor_slabs.end(); ++it) {
    if (it->tile.x == slab.tile.x && it->tile.z == slab.tile.z &&
        std::fabs(it->top_y - slab.top_y) < kSameTopY) {
      map.floor_slabs.erase(it);
      return ok_result();
    }
  }
  map.floor_slabs.push_back(std::move(slab));
  return ok_result();
}

}  // namespace rat
