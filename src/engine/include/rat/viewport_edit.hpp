#pragma once

#include "rat/camera.hpp"
#include "rat/map_data.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace rat {

enum class ViewportTool {
  Select,
  PlaceBlocker,
  PlaceEvent,
  PlaceCube,
  PlaceFence,
  PlaceSlab,
  PlaceLadder,
};

enum class ViewportPickKind {
  Blocker,
  Event,
};

struct ViewportPick {
  ViewportPickKind kind = ViewportPickKind::Blocker;
  std::size_t index = 0;
};

enum class ViewportClickActionKind {
  None,
  Deselect,
  SelectBlocker,
  SelectEvent,
  PlaceBlocker,
  PlaceEvent,
  PlaceCube,
  PlaceFence,
  PlaceSlab,
  PlaceLadder,
};

struct ViewportClickAction {
  ViewportClickActionKind kind = ViewportClickActionKind::None;
  std::size_t index = 0;
  TileCoord tile{};
};

struct TileDelta {
  int tile_dx = 0;
  int tile_dz = 0;
};

[[nodiscard]] std::optional<Vec3> unproject_to_ground_plane(const OrthoCamera& camera, float pixel_x,
                                                            float pixel_y,
                                                            std::uint32_t framebuffer_width,
                                                            std::uint32_t framebuffer_height,
                                                            float ground_y = 0.0f);

// Picks only objects under the hit point; if several overlap, chooses nearest center.
// Tie is resolved by preferring blockers.
[[nodiscard]] std::optional<ViewportPick> pick_map_object_xz(const MapData& map, Vec3 world_hit);

[[nodiscard]] TileCoord world_to_tile_xz(Vec3 world_hit, float tile_size);
[[nodiscard]] TileDelta tile_delta_between(TileCoord from, TileCoord to);

[[nodiscard]] ViewportClickAction resolve_viewport_click(const MapData& map, ViewportTool tool,
                                                         Vec3 world_hit);

}  // namespace rat
