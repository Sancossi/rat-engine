#pragma once

#include "rat/map_data.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace rat {

[[nodiscard]] Vec3 tile_center_world(TileCoord tile, float tile_size);
[[nodiscard]] EventDef make_stub_event(std::string id, int tile_x, int tile_z);

[[nodiscard]] std::string allocate_unique_event_id(const MapData& map, std::string_view base);
[[nodiscard]] EventDef make_duplicate_event(const MapData& map, std::size_t index);

// Moves tile coords and/or volume AABB by whole tiles.
void translate_event_on_grid(EventDef& event, int tile_dx, int tile_dz, float tile_size);

// Marker list index for an event that has tile or volume; -1 if none.
[[nodiscard]] int event_marker_index(const MapData& map, int event_index);

}  // namespace rat
