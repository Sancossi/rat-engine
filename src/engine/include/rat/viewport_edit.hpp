#pragma once

#include "rat/camera.hpp"
#include "rat/map_data.hpp"
#include "rat/terrain_geometry.hpp"

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
  PlaceBridge,
  PlaceLadder,
  PlaceRamp,
};

enum class EditSubmode {
  Terrain,
  Objects,
  Events,
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
  PlaceBridge,
  PlaceLadder,
  PlaceRamp,
};

struct ViewportClickAction {
  ViewportClickActionKind kind = ViewportClickActionKind::None;
  std::size_t index = 0;
  TileCoord tile{};
  RampDirection edge = RampDirection::North;
};

struct TileDelta {
  int tile_dx = 0;
  int tile_dz = 0;
};

struct PixelPos {
  float x = 0.0f;
  float y = 0.0f;
};

[[nodiscard]] std::optional<PixelPos> project_world_to_pixels(const OrthoCamera& camera, Vec3 world,
                                                            std::uint32_t framebuffer_width,
                                                            std::uint32_t framebuffer_height);

[[nodiscard]] std::optional<Vec3> unproject_to_ground_plane(const OrthoCamera& camera, float pixel_x,
                                                            float pixel_y,
                                                            std::uint32_t framebuffer_width,
                                                            std::uint32_t framebuffer_height,
                                                            float ground_y = 0.0f);

// Pixel ray vs terrain tile quads (ramps and raised cubes). Empty geometry falls back to y=0.
[[nodiscard]] std::optional<Vec3> unproject_to_terrain(const OrthoCamera& camera, float pixel_x,
                                                       float pixel_y,
                                                       std::uint32_t framebuffer_width,
                                                       std::uint32_t framebuffer_height,
                                                       const TerrainGeometry& geometry);

// Terrain never picks objects. Objects pick blockers only. Events pick events only.
// If several of the allowed kind overlap, chooses nearest center.
[[nodiscard]] std::optional<ViewportPick> pick_map_object_xz(const MapData& map, Vec3 world_hit,
                                                            EditSubmode submode);

[[nodiscard]] TileCoord world_to_tile_xz(Vec3 world_hit, float tile_size);
[[nodiscard]] RampDirection nearest_tile_edge(Vec3 world_hit, float tile_size);
[[nodiscard]] TileDelta tile_delta_between(TileCoord from, TileCoord to);

[[nodiscard]] ViewportClickAction resolve_viewport_click(const MapData& map, ViewportTool tool,
                                                         Vec3 world_hit, EditSubmode submode);

[[nodiscard]] bool viewport_tool_allowed(EditSubmode submode, ViewportTool tool);

}  // namespace rat
