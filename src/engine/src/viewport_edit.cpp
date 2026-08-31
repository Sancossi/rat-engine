#include "rat/viewport_edit.hpp"

#include "rat/blocker_edit.hpp"
#include "rat/event_edit.hpp"

#include <algorithm>
#include <array>
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

std::array<float, 4> mul_mat4_vec4(const Mat4& m, const std::array<float, 4>& v) {
  std::array<float, 4> out{};
  for (int row = 0; row < 4; ++row) {
    out[static_cast<std::size_t>(row)] = m.m[0 * 4 + row] * v[0] + m.m[1 * 4 + row] * v[1] +
                                         m.m[2 * 4 + row] * v[2] + m.m[3 * 4 + row] * v[3];
  }
  return out;
}

Mat4 invert_rigid_transform(const Mat4& m) {
  Mat4 out{};
  const float r00 = m.m[0];
  const float r01 = m.m[4];
  const float r02 = m.m[8];
  const float r10 = m.m[1];
  const float r11 = m.m[5];
  const float r12 = m.m[9];
  const float r20 = m.m[2];
  const float r21 = m.m[6];
  const float r22 = m.m[10];

  out.m[0] = r00;
  out.m[1] = r01;
  out.m[2] = r02;
  out.m[4] = r10;
  out.m[5] = r11;
  out.m[6] = r12;
  out.m[8] = r20;
  out.m[9] = r21;
  out.m[10] = r22;

  const float tx = m.m[12];
  const float ty = m.m[13];
  const float tz = m.m[14];
  out.m[12] = -(out.m[0] * tx + out.m[4] * ty + out.m[8] * tz);
  out.m[13] = -(out.m[1] * tx + out.m[5] * ty + out.m[9] * tz);
  out.m[14] = -(out.m[2] * tx + out.m[6] * ty + out.m[10] * tz);
  out.m[15] = 1.0f;
  return out;
}

std::optional<Vec3> unproject_ndc(const Mat4& proj, const Mat4& inv_view, float ndc_x, float ndc_y,
                                  float ndc_z) {
  if (std::fabs(proj.m[0]) <= kEpsilon || std::fabs(proj.m[5]) <= kEpsilon ||
      std::fabs(proj.m[10]) <= kEpsilon) {
    return std::nullopt;
  }
  const float view_x = (ndc_x - proj.m[12]) / proj.m[0];
  const float view_y = (ndc_y - proj.m[13]) / proj.m[5];
  const float view_z = (ndc_z - proj.m[14]) / proj.m[10];
  const std::array<float, 4> view_pos = {view_x, view_y, view_z, 1.0f};
  const std::array<float, 4> world = mul_mat4_vec4(inv_view, view_pos);
  if (std::fabs(world[3]) <= kEpsilon) {
    return std::nullopt;
  }
  const float inv_world_w = 1.0f / world[3];
  return Vec3{world[0] * inv_world_w, world[1] * inv_world_w, world[2] * inv_world_w};
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

  const Mat4 inv_view = invert_rigid_transform(camera.view);

  const std::optional<Vec3> near_hit = unproject_ndc(camera.proj, inv_view, ndc_x, ndc_y, -1.0f);
  const std::optional<Vec3> far_hit = unproject_ndc(camera.proj, inv_view, ndc_x, ndc_y, 1.0f);
  if (!near_hit.has_value() || !far_hit.has_value()) {
    return std::nullopt;
  }

  const Vec3 ray_dir{
      far_hit->x - near_hit->x,
      far_hit->y - near_hit->y,
      far_hit->z - near_hit->z,
  };
  if (std::fabs(ray_dir.y) <= kEpsilon) {
    return std::nullopt;
  }

  const float t = (ground_y - near_hit->y) / ray_dir.y;
  if (t < 0.0f) {
    return std::nullopt;
  }
  const float world_x = near_hit->x + ray_dir.x * t;
  const float world_z = near_hit->z + ray_dir.z * t;
  // Camera view space uses opposite Z sign versus map tile coordinates in editor tools.
  return Vec3{world_x, ground_y, -world_z};
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
