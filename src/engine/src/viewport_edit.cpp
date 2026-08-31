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

}  // namespace

std::optional<Vec3> unproject_to_ground_plane(const OrthoCamera& camera, float pixel_x, float pixel_y,
                                              std::uint32_t framebuffer_width,
                                              std::uint32_t framebuffer_height, float ground_y) {
  const float width = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_width));
  const float height = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_height));
  const float ndc_x = 2.0f * pixel_x / width - 1.0f;
  const float ndc_y = 1.0f - 2.0f * pixel_y / height;
  if (std::fabs(camera.proj.m[0]) <= kEpsilon || std::fabs(camera.proj.m[5]) <= kEpsilon) {
    return std::nullopt;
  }

  const Vec3 right{camera.view.m[0], camera.view.m[1], camera.view.m[2]};
  const Vec3 up{camera.view.m[4], camera.view.m[5], camera.view.m[6]};
  const Vec3 minus_forward{camera.view.m[8], camera.view.m[9], camera.view.m[10]};
  const Vec3 forward{-minus_forward.x, -minus_forward.y, -minus_forward.z};

  // camera.cpp look_at stores translation as:
  // tx = -dot(right, eye), ty = -dot(up, eye), tz = dot(forward, eye).
  // Recover eye in world space from that identity:
  // eye = -tx * right - ty * up + tz * forward.
  const Vec3 eye{
      -camera.view.m[12] * right.x - camera.view.m[13] * up.x + camera.view.m[14] * forward.x,
      -camera.view.m[12] * right.y - camera.view.m[13] * up.y + camera.view.m[14] * forward.y,
      -camera.view.m[12] * right.z - camera.view.m[13] * up.z + camera.view.m[14] * forward.z,
  };

  const float view_x = (ndc_x - camera.proj.m[12]) / camera.proj.m[0];
  const float view_y = (ndc_y - camera.proj.m[13]) / camera.proj.m[5];
  const Vec3 ray_origin{
      eye.x + right.x * view_x + up.x * view_y,
      eye.y + right.y * view_x + up.y * view_y,
      eye.z + right.z * view_x + up.z * view_y,
  };

  if (std::fabs(forward.y) <= kEpsilon) {
    return std::nullopt;
  }

  const float t = (ground_y - ray_origin.y) / forward.y;
  if (t < 0.0f) {
    return std::nullopt;
  }
  return Vec3{
      ray_origin.x + forward.x * t,
      ground_y,
      ray_origin.z + forward.z * t,
  };
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
  switch (tool) {
    case ViewportTool::Select:
      return {ViewportClickActionKind::Deselect, 0, tile};
    case ViewportTool::PlaceBlocker:
      return {ViewportClickActionKind::PlaceBlocker, 0, tile};
    case ViewportTool::PlaceEvent:
      return {ViewportClickActionKind::PlaceEvent, 0, tile};
  }
  return {};
}

}  // namespace rat
