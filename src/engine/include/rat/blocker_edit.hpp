#pragma once

#include "rat/map_data.hpp"

#include <optional>

namespace rat {

enum class AabbEdge {
  MinX,
  MaxX,
  MinZ,
  MaxZ,
};

[[nodiscard]] Aabb2 normalize_aabb(Aabb2 box);
[[nodiscard]] Aabb2 snap_aabb_to_grid(Aabb2 box, float tile_size);
[[nodiscard]] Aabb2 translate_aabb_on_grid(Aabb2 box, int tile_dx, int tile_dz, float tile_size);
[[nodiscard]] Aabb2 resize_aabb_on_grid(Aabb2 box, AabbEdge edge, int tile_delta, float tile_size);
[[nodiscard]] bool set_blocker_vertical_range(BlockerDef& blocker, bool jumpable,
                                              std::optional<float> base_y,
                                              std::optional<float> top_y);

}  // namespace rat
