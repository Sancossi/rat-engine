#pragma once

#include "rat/camera.hpp"
#include "rat/map_data.hpp"
#include "rat/terrain_geometry.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

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
  PlaceVoxel,
  RemoveVoxel,
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
  PlaceVoxel,
  RemoveVoxel,
};

struct ViewportClickAction {
  ViewportClickActionKind kind = ViewportClickActionKind::None;
  std::size_t index = 0;
  TileCoord tile{};
  RampDirection edge = RampDirection::North;
  int voxel_y = 0;
};

struct VoxelCoord {
  int x = 0;
  int y = 0;
  int z = 0;
};

enum class VoxelFace {
  PosX,
  NegX,
  PosY,
  NegY,
  PosZ,
  NegZ,
};

struct OccupancyFaceHit {
  int x = 0;
  int y = 0;
  int z = 0;
  VoxelFace face = VoxelFace::PosY;
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

// Pixel ray vs occupancy solid AABBs. Empty if no solid is hit.
[[nodiscard]] std::optional<Vec3> unproject_to_occupancy(const OrthoCamera& camera, float pixel_x,
                                                         float pixel_y,
                                                         std::uint32_t framebuffer_width,
                                                         std::uint32_t framebuffer_height,
                                                         std::span<const OccupancyCell> occupancy,
                                                         float tile_size);

// Terrain never picks objects. Objects pick blockers only. Events pick events only.
// If several of the allowed kind overlap, chooses nearest center.
[[nodiscard]] std::optional<ViewportPick> pick_map_object_xz(const MapData& map, Vec3 world_hit,
                                                            EditSubmode submode);

[[nodiscard]] TileCoord world_to_tile_xz(Vec3 world_hit, float tile_size);
[[nodiscard]] RampDirection nearest_tile_edge(Vec3 world_hit, float tile_size);
[[nodiscard]] TileDelta tile_delta_between(TileCoord from, TileCoord to);

[[nodiscard]] ViewportClickAction resolve_viewport_click(const MapData& map, ViewportTool tool,
                                                         Vec3 world_hit, EditSubmode submode,
                                                         int voxel_layer = 0);

[[nodiscard]] bool viewport_tool_allowed(EditSubmode submode, ViewportTool tool);

[[nodiscard]] VoxelCoord adjacent_voxel(VoxelCoord cell, VoxelFace face);
[[nodiscard]] std::optional<OccupancyFaceHit> pick_occupancy_face_at(
    std::span<const OccupancyCell> occupancy, Vec3 world_hit, float tile_size);
[[nodiscard]] VoxelCoord voxel_cell_for_place(const MapData& map, Vec3 world_hit, int layer_y);
[[nodiscard]] VoxelCoord voxel_cell_for_remove(const MapData& map, Vec3 world_hit, int layer_y);

}  // namespace rat
