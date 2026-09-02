#include "rat/viewport_edit.hpp"

#include "rat/blocker_edit.hpp"
#include "rat/event_edit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rat {
namespace {

constexpr float kEpsilon = 1e-6f;

float safe_tile_size(float tile_size) {
  return tile_size > kEpsilon ? tile_size : 1.0f;
}

Vec3 center_of_aabb(Aabb2 box) {
  box = normalize_aabb(box);
  return {0.5f * (box.min_x + box.max_x), 0.0f, 0.5f * (box.min_z + box.max_z)};
}

bool contains_xz(Aabb2 box, Vec3 point) {
  box = normalize_aabb(box);
  return point.x >= box.min_x && point.x <= box.max_x && point.z >= box.min_z && point.z <= box.max_z;
}

float dist2_xz(Vec3 a, Vec3 b) {
  const float dx = a.x - b.x;
  const float dz = a.z - b.z;
  return dx * dx + dz * dz;
}

bool event_contains_point(const EventDef& event, float tile_size, Vec3 point) {
  if (event.tile.has_value()) {
    const float base_x = static_cast<float>(event.tile->x) * tile_size;
    const float base_z = static_cast<float>(event.tile->z) * tile_size;
    const Aabb2 tile_box{base_x, base_z, base_x + tile_size, base_z + tile_size};
    return contains_xz(tile_box, point);
  }
  if (event.volume.has_value()) {
    return contains_xz(*event.volume, point);
  }
  return false;
}

Vec3 event_center(const EventDef& event, float tile_size) {
  if (event.tile.has_value()) {
    return tile_center_world(*event.tile, tile_size);
  }
  if (event.volume.has_value()) {
    return center_of_aabb(*event.volume);
  }
  return {};
}

void mul_mat4(const float a[16], const float b[16], float out[16]) {
  float tmp[16];
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      tmp[col * 4 + row] = a[0 * 4 + row] * b[col * 4 + 0] + a[1 * 4 + row] * b[col * 4 + 1] +
                           a[2 * 4 + row] * b[col * 4 + 2] + a[3 * 4 + row] * b[col * 4 + 3];
    }
  }
  for (int i = 0; i < 16; ++i) {
    out[i] = tmp[i];
  }
}

bool invert_mat4(const float m[16], float inv_out[16]) {
  float inv[16];
  inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
           m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
  inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
           m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
  inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
           m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
  inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
            m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
  inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
           m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
  inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
           m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
  inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
           m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
  inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
            m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
  inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
           m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
  inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
           m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
  inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
            m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
  inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
            m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
  inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
           m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
  inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
           m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
  inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
            m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
  inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
            m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

  const float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
  if (std::fabs(det) <= kEpsilon) {
    return false;
  }
  const float inv_det = 1.0f / det;
  for (int i = 0; i < 16; ++i) {
    inv_out[i] = inv[i] * inv_det;
  }
  return true;
}

std::optional<Vec3> transform_clip_to_world(const float inv_clip[16], float ndc_x, float ndc_y,
                                            float ndc_z) {
  const float x = inv_clip[0] * ndc_x + inv_clip[4] * ndc_y + inv_clip[8] * ndc_z + inv_clip[12];
  const float y = inv_clip[1] * ndc_x + inv_clip[5] * ndc_y + inv_clip[9] * ndc_z + inv_clip[13];
  const float z = inv_clip[2] * ndc_x + inv_clip[6] * ndc_y + inv_clip[10] * ndc_z + inv_clip[14];
  const float w = inv_clip[3] * ndc_x + inv_clip[7] * ndc_y + inv_clip[11] * ndc_z + inv_clip[15];
  if (std::fabs(w) <= kEpsilon) {
    return std::nullopt;
  }
  return Vec3{x / w, y / w, z / w};
}

}  // namespace

std::optional<Vec3> unproject_to_ground_plane(const OrthoCamera& camera, float pixel_x, float pixel_y,
                                              std::uint32_t framebuffer_width,
                                              std::uint32_t framebuffer_height, float ground_y) {
  // Invert live view/proj as opaque world-to-clip 4x4 (what bgfx::setViewTransform consumes).
  const float width = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_width));
  const float height = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_height));
  const float ndc_x = 2.0f * pixel_x / width - 1.0f;
  const float ndc_y = 1.0f - 2.0f * pixel_y / height;

  float clip[16];
  mul_mat4(camera.proj.m, camera.view.m, clip);
  float inv_clip[16];
  if (!invert_mat4(clip, inv_clip)) {
    return std::nullopt;
  }

  const auto p0 = transform_clip_to_world(inv_clip, ndc_x, ndc_y, -1.0f);
  const auto p1 = transform_clip_to_world(inv_clip, ndc_x, ndc_y, 1.0f);
  if (!p0.has_value() || !p1.has_value()) {
    return std::nullopt;
  }

  const float dy = p1->y - p0->y;
  if (std::fabs(dy) <= kEpsilon) {
    return std::nullopt;
  }
  const float t = (ground_y - p0->y) / dy;
  return Vec3{p0->x + (p1->x - p0->x) * t, ground_y, p0->z + (p1->z - p0->z) * t};
}

std::optional<ViewportPick> pick_map_object_xz(const MapData& map, Vec3 world_hit) {
  const float tile = safe_tile_size(map.tile_size);
  bool found = false;
  ViewportPick best{};
  float best_dist2 = std::numeric_limits<float>::infinity();

  for (std::size_t i = 0; i < map.blockers.size(); ++i) {
    const BlockerDef& blocker = map.blockers[i];
    if (!contains_xz(blocker.bounds, world_hit)) {
      continue;
    }
    const float d2 = dist2_xz(center_of_aabb(blocker.bounds), world_hit);
    if (!found || d2 < best_dist2) {
      found = true;
      best = {ViewportPickKind::Blocker, i};
      best_dist2 = d2;
    }
  }

  for (std::size_t i = 0; i < map.events.size(); ++i) {
    const EventDef& event = map.events[i];
    if (!event_contains_point(event, tile, world_hit)) {
      continue;
    }
    const float d2 = dist2_xz(event_center(event, tile), world_hit);
    if (!found || d2 < best_dist2) {
      found = true;
      best = {ViewportPickKind::Event, i};
      best_dist2 = d2;
    }
  }

  if (!found) {
    return std::nullopt;
  }
  return best;
}

TileCoord world_to_tile_xz(Vec3 world_hit, float tile_size) {
  const float tile = safe_tile_size(tile_size);
  return {
      static_cast<int>(std::floor(world_hit.x / tile)),
      static_cast<int>(std::floor(world_hit.z / tile)),
  };
}

RampDirection nearest_tile_edge(Vec3 world_hit, float tile_size) {
  const float tile = safe_tile_size(tile_size);
  const TileCoord coord = world_to_tile_xz(world_hit, tile);
  const float ox = static_cast<float>(coord.x) * tile;
  const float oz = static_cast<float>(coord.z) * tile;
  const float dist_west = world_hit.x - ox;
  const float dist_east = (ox + tile) - world_hit.x;
  const float dist_north = world_hit.z - oz;
  const float dist_south = (oz + tile) - world_hit.z;

  RampDirection best = RampDirection::North;
  float best_dist = dist_north;
  const auto consider = [&](RampDirection direction, float dist) {
    if (dist < best_dist) {
      best_dist = dist;
      best = direction;
    }
  };
  consider(RampDirection::East, dist_east);
  consider(RampDirection::South, dist_south);
  consider(RampDirection::West, dist_west);
  return best;
}

TileDelta tile_delta_between(TileCoord from, TileCoord to) {
  return {
      to.x - from.x,
      to.z - from.z,
  };
}

ViewportClickAction resolve_viewport_click(const MapData& map, ViewportTool tool, Vec3 world_hit) {
  if (const auto picked = pick_map_object_xz(map, world_hit); picked.has_value()) {
    if (picked->kind == ViewportPickKind::Blocker) {
      return {ViewportClickActionKind::SelectBlocker, picked->index, {}};
    }
    return {ViewportClickActionKind::SelectEvent, picked->index, {}};
  }

  const TileCoord tile = world_to_tile_xz(world_hit, map.tile_size);
  const RampDirection edge = nearest_tile_edge(world_hit, map.tile_size);
  switch (tool) {
    case ViewportTool::Select:
      return {ViewportClickActionKind::Deselect, 0, tile, edge};
    case ViewportTool::PlaceBlocker:
      return {ViewportClickActionKind::PlaceBlocker, 0, tile, edge};
    case ViewportTool::PlaceEvent:
      return {ViewportClickActionKind::PlaceEvent, 0, tile, edge};
    case ViewportTool::PlaceCube:
      return {ViewportClickActionKind::PlaceCube, 0, tile, edge};
    case ViewportTool::PlaceFence:
      return {ViewportClickActionKind::PlaceFence, 0, tile, edge};
    case ViewportTool::PlaceSlab:
      return {ViewportClickActionKind::PlaceSlab, 0, tile, edge};
    case ViewportTool::PlaceLadder:
      return {ViewportClickActionKind::PlaceLadder, 0, tile, edge};
  }
  return {};
}

}  // namespace rat
