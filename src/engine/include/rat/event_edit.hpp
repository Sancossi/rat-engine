#pragma once

#include "rat/map_data.hpp"

#include <string>

namespace rat {

[[nodiscard]] EventDef make_stub_event(std::string id, int tile_x, int tile_z);

// Moves tile coords and/or volume AABB by whole tiles.
void translate_event_on_grid(EventDef& event, int tile_dx, int tile_dz, float tile_size);

// Marker list index for an event that has tile or volume; -1 if none.
[[nodiscard]] int event_marker_index(const MapData& map, int event_index);

}  // namespace rat
