#include "rat/viewport_edit.hpp"

#include "rat/blocker_edit.hpp"
#include "rat/event_edit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <utility>

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

void mul_mat4_vec4(const float m[16], float x, float y, float z, float w, float out[4]) {
  out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
  out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
  out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
  out[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;
}

struct CameraRay {
  Vec3 near_world{};
  Vec3 far_world{};
};

Vec3 vec_sub(Vec3 a, Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 vec_cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float vec_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

std::optional<CameraRay> unproject_camera_ray(const OrthoCamera& camera, float pixel_x,
                                              float pixel_y, std::uint32_t framebuffer_width,
                                              std::uint32_t framebuffer_height) {
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
  return CameraRay{*p0, *p1};
}

std::optional<Vec3> intersect_ray_ground_y(const CameraRay& ray, float ground_y) {
  const float dy = ray.far_world.y - ray.near_world.y;
  if (std::fabs(dy) <= kEpsilon) {
    return std::nullopt;
  }
  const float t = (ground_y - ray.near_world.y) / dy;
  return Vec3{ray.near_world.x + (ray.far_world.x - ray.near_world.x) * t, ground_y,
              ray.near_world.z + (ray.far_world.z - ray.near_world.z) * t};
}

std::optional<float> ray_triangle_t(Vec3 origin, Vec3 dir, Vec3 v0, Vec3 v1, Vec3 v2) {
  const Vec3 edge1 = vec_sub(v1, v0);
  const Vec3 edge2 = vec_sub(v2, v0);
  const Vec3 h = vec_cross(dir, edge2);
  const float a = vec_dot(edge1, h);
  if (std::fabs(a) <= kEpsilon) {
    return std::nullopt;
  }
  const float f = 1.0f / a;
  const Vec3 s = vec_sub(origin, v0);
  const float u = f * vec_dot(s, h);
  if (u < -kEpsilon || u > 1.0f + kEpsilon) {
    return std::nullopt;
  }
  const Vec3 q = vec_cross(s, edge1);
  const float v = f * vec_dot(dir, q);
  if (v < -kEpsilon || u + v > 1.0f + kEpsilon) {
    return std::nullopt;
  }
  const float t = f * vec_dot(edge2, q);
  return t;
}

void consider_triangle(const CameraRay& ray, Vec3 eye, Vec3 v0, Vec3 v1, Vec3 v2, bool& found,
                       float& best_dist2, Vec3& best_hit) {
  const Vec3 dir = vec_sub(ray.far_world, ray.near_world);
  const auto t = ray_triangle_t(ray.near_world, dir, v0, v1, v2);
  if (!t.has_value()) {
    return;
  }
  const Vec3 hit{ray.near_world.x + dir.x * *t, ray.near_world.y + dir.y * *t,
                 ray.near_world.z + dir.z * *t};
  const float dist2 = vec_dot(vec_sub(hit, eye), vec_sub(hit, eye));
  if (found && dist2 >= best_dist2) {
    return;
  }
  found = true;
  best_dist2 = dist2;
  best_hit = hit;
}

}  // namespace

std::optional<Vec3> unproject_to_ground_plane(const OrthoCamera& camera, float pixel_x, float pixel_y,
                                              std::uint32_t framebuffer_width,
                                              std::uint32_t framebuffer_height, float ground_y) {
  const auto ray = unproject_camera_ray(camera, pixel_x, pixel_y, framebuffer_width, framebuffer_height);
  if (!ray.has_value()) {
    return std::nullopt;
  }
  return intersect_ray_ground_y(*ray, ground_y);
}

std::optional<Vec3> unproject_to_terrain(const OrthoCamera& camera, float pixel_x, float pixel_y,
                                         std::uint32_t framebuffer_width,
                                         std::uint32_t framebuffer_height,
                                         const TerrainGeometry& geometry) {
  const auto ray = unproject_camera_ray(camera, pixel_x, pixel_y, framebuffer_width, framebuffer_height);
  if (!ray.has_value()) {
    return std::nullopt;
  }
  if (geometry.tiles.empty()) {
    return intersect_ray_ground_y(*ray, 0.0f);
  }

  bool found = false;
  float best_dist2 = 0.0f;
  Vec3 best_hit{};
  for (const TerrainTileQuad& tile : geometry.tiles) {
    const Vec3 nw{tile.min_x, tile.y_nw, tile.min_z};
    const Vec3 ne{tile.max_x, tile.y_ne, tile.min_z};
    const Vec3 se{tile.max_x, tile.y_se, tile.max_z};
    const Vec3 sw{tile.min_x, tile.y_sw, tile.max_z};
    consider_triangle(*ray, camera.eye, nw, ne, se, found, best_dist2, best_hit);
    consider_triangle(*ray, camera.eye, nw, se, sw, found, best_dist2, best_hit);
  }
  if (found) {
    return best_hit;
  }
  return intersect_ray_ground_y(*ray, 0.0f);
}

namespace {

std::optional<float> ray_aabb_t(Vec3 origin, Vec3 dir, Vec3 bmin, Vec3 bmax) {
  float tmin = 0.0f;
  float tmax = 1.0f;
  const float origin_v[3] = {origin.x, origin.y, origin.z};
  const float dir_v[3] = {dir.x, dir.y, dir.z};
  const float min_v[3] = {bmin.x, bmin.y, bmin.z};
  const float max_v[3] = {bmax.x, bmax.y, bmax.z};
  for (int i = 0; i < 3; ++i) {
    if (std::fabs(dir_v[i]) <= kEpsilon) {
      if (origin_v[i] < min_v[i] || origin_v[i] > max_v[i]) {
        return std::nullopt;
      }
      continue;
    }
    float t1 = (min_v[i] - origin_v[i]) / dir_v[i];
    float t2 = (max_v[i] - origin_v[i]) / dir_v[i];
    if (t1 > t2) {
      std::swap(t1, t2);
    }
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    if (tmin > tmax) {
      return std::nullopt;
    }
  }
  return tmin;
}

VoxelFace aabb_hit_face(Vec3 hit, Vec3 bmin, Vec3 bmax) {
  const float dx_pos = std::fabs(hit.x - bmax.x);
  const float dx_neg = std::fabs(hit.x - bmin.x);
  const float dy_pos = std::fabs(hit.y - bmax.y);
  const float dy_neg = std::fabs(hit.y - bmin.y);
  const float dz_pos = std::fabs(hit.z - bmax.z);
  const float dz_neg = std::fabs(hit.z - bmin.z);
  float best = dx_pos;
  VoxelFace face = VoxelFace::PosX;
  const auto consider = [&](VoxelFace next, float dist) {
    if (dist < best) {
      best = dist;
      face = next;
    }
  };
  consider(VoxelFace::NegX, dx_neg);
  consider(VoxelFace::PosY, dy_pos);
  consider(VoxelFace::NegY, dy_neg);
  consider(VoxelFace::PosZ, dz_pos);
  consider(VoxelFace::NegZ, dz_neg);
  return face;
}

void occupancy_cell_aabb(const OccupancyCell& cell, float tile_size, Vec3& bmin, Vec3& bmax) {
  const float ts = safe_tile_size(tile_size);
  bmin = {static_cast<float>(cell.x) * ts, static_cast<float>(cell.y) * ts,
          static_cast<float>(cell.z) * ts};
  bmax = {bmin.x + ts, bmin.y + ts, bmin.z + ts};
}

}  // namespace

std::optional<Vec3> unproject_to_occupancy(const OrthoCamera& camera, float pixel_x, float pixel_y,
                                           std::uint32_t framebuffer_width,
                                           std::uint32_t framebuffer_height,
                                           std::span<const OccupancyCell> occupancy,
                                           float tile_size) {
  const auto ray = unproject_camera_ray(camera, pixel_x, pixel_y, framebuffer_width, framebuffer_height);
  if (!ray.has_value()) {
    return std::nullopt;
  }
  const Vec3 dir = vec_sub(ray->far_world, ray->near_world);
  bool found = false;
  float best_dist2 = 0.0f;
  Vec3 best_hit{};
  for (const OccupancyCell& cell : occupancy) {
    Vec3 bmin{};
    Vec3 bmax{};
    occupancy_cell_aabb(cell, tile_size, bmin, bmax);
    const auto t = ray_aabb_t(ray->near_world, dir, bmin, bmax);
    if (!t.has_value()) {
      continue;
    }
    const Vec3 hit{ray->near_world.x + dir.x * *t, ray->near_world.y + dir.y * *t,
                   ray->near_world.z + dir.z * *t};
    const float dist2 = vec_dot(vec_sub(hit, camera.eye), vec_sub(hit, camera.eye));
    if (found && dist2 >= best_dist2) {
      continue;
    }
    found = true;
    best_dist2 = dist2;
    best_hit = hit;
  }
  if (!found) {
    return std::nullopt;
  }
  return best_hit;
}

std::optional<PixelPos> project_world_to_pixels(const OrthoCamera& camera, Vec3 world,
                                               std::uint32_t framebuffer_width,
                                               std::uint32_t framebuffer_height) {
  const float width = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_width));
  const float height = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_height));

  float clip[16];
  mul_mat4(camera.proj.m, camera.view.m, clip);

  float clip_pos[4];
  mul_mat4_vec4(clip, world.x, world.y, world.z, 1.0f, clip_pos);
  if (std::fabs(clip_pos[3]) <= kEpsilon || clip_pos[3] < 0.0f) {
    return std::nullopt;
  }

  const float inv_w = 1.0f / clip_pos[3];
  const float ndc_x = clip_pos[0] * inv_w;
  const float ndc_y = clip_pos[1] * inv_w;
  const float pixel_x = (ndc_x + 1.0f) * 0.5f * width;
  const float pixel_y = (1.0f - ndc_y) * 0.5f * height;
  return PixelPos{pixel_x, pixel_y};
}

std::optional<ViewportPick> pick_map_object_xz(const MapData& map, Vec3 world_hit,
                                              EditSubmode submode) {
  if (submode == EditSubmode::Terrain) {
    return std::nullopt;
  }

  const float tile = safe_tile_size(map.tile_size);
  bool found = false;
  ViewportPick best{};
  float best_dist2 = std::numeric_limits<float>::infinity();

  if (submode == EditSubmode::Objects) {
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
  }

  if (submode == EditSubmode::Events) {
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

ViewportClickAction resolve_viewport_click(const MapData& map, ViewportTool tool, Vec3 world_hit,
                                           EditSubmode submode, int voxel_layer) {
  if (const auto picked = pick_map_object_xz(map, world_hit, submode); picked.has_value()) {
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
    case ViewportTool::PlaceBridge:
      return {ViewportClickActionKind::PlaceBridge, 0, tile, edge};
    case ViewportTool::PlaceLadder:
      return {ViewportClickActionKind::PlaceLadder, 0, tile, edge};
    case ViewportTool::PlaceRamp:
      return {ViewportClickActionKind::PlaceRamp, 0, tile, edge};
    case ViewportTool::PlaceVoxel: {
      const VoxelCoord cell = voxel_cell_for_place(map, world_hit, voxel_layer);
      return {ViewportClickActionKind::PlaceVoxel, 0, TileCoord{cell.x, cell.z}, edge, cell.y};
    }
    case ViewportTool::PlaceVoxelRamp: {
      const VoxelCoord cell = voxel_cell_for_place(map, world_hit, voxel_layer);
      return {ViewportClickActionKind::PlaceVoxelRamp, 0, TileCoord{cell.x, cell.z}, edge, cell.y};
    }
    case ViewportTool::RemoveVoxel: {
      const VoxelCoord cell = voxel_cell_for_remove(map, world_hit, voxel_layer);
      return {ViewportClickActionKind::RemoveVoxel, 0, TileCoord{cell.x, cell.z}, edge, cell.y};
    }
  }
  return {};
}

bool viewport_tool_allowed(EditSubmode submode, ViewportTool tool) {
  switch (submode) {
    case EditSubmode::Terrain:
      return tool == ViewportTool::Select || tool == ViewportTool::PlaceCube ||
             tool == ViewportTool::PlaceFence || tool == ViewportTool::PlaceSlab ||
             tool == ViewportTool::PlaceBridge || tool == ViewportTool::PlaceRamp ||
             tool == ViewportTool::PlaceVoxel || tool == ViewportTool::PlaceVoxelRamp ||
             tool == ViewportTool::RemoveVoxel;
    case EditSubmode::Objects:
      return tool == ViewportTool::Select || tool == ViewportTool::PlaceBlocker ||
             tool == ViewportTool::PlaceLadder;
    case EditSubmode::Events:
      return tool == ViewportTool::Select || tool == ViewportTool::PlaceEvent;
  }
  return false;
}

VoxelCoord adjacent_voxel(VoxelCoord cell, VoxelFace face) {
  switch (face) {
    case VoxelFace::PosX:
      cell.x += 1;
      break;
    case VoxelFace::NegX:
      cell.x -= 1;
      break;
    case VoxelFace::PosY:
      cell.y += 1;
      break;
    case VoxelFace::NegY:
      cell.y -= 1;
      break;
    case VoxelFace::PosZ:
      cell.z += 1;
      break;
    case VoxelFace::NegZ:
      cell.z -= 1;
      break;
  }
  return cell;
}

std::optional<OccupancyFaceHit> pick_occupancy_face_at(std::span<const OccupancyCell> occupancy,
                                                       Vec3 world_hit, float tile_size) {
  constexpr float kFaceEps = 0.05f;
  bool found = false;
  float best_dist = 0.0f;
  OccupancyFaceHit best{};
  for (const OccupancyCell& cell : occupancy) {
    Vec3 bmin{};
    Vec3 bmax{};
    occupancy_cell_aabb(cell, tile_size, bmin, bmax);
    if (world_hit.x < bmin.x - kFaceEps || world_hit.x > bmax.x + kFaceEps ||
        world_hit.y < bmin.y - kFaceEps || world_hit.y > bmax.y + kFaceEps ||
        world_hit.z < bmin.z - kFaceEps || world_hit.z > bmax.z + kFaceEps) {
      continue;
    }
    const VoxelFace face = aabb_hit_face(world_hit, bmin, bmax);
    const float dist_x = std::min(std::fabs(world_hit.x - bmin.x), std::fabs(world_hit.x - bmax.x));
    const float dist_y = std::min(std::fabs(world_hit.y - bmin.y), std::fabs(world_hit.y - bmax.y));
    const float dist_z = std::min(std::fabs(world_hit.z - bmin.z), std::fabs(world_hit.z - bmax.z));
    const float dist = std::min(dist_x, std::min(dist_y, dist_z));
    if (dist > kFaceEps) {
      continue;
    }
    if (found && dist >= best_dist) {
      continue;
    }
    found = true;
    best_dist = dist;
    best.x = cell.x;
    best.y = cell.y;
    best.z = cell.z;
    best.face = face;
  }
  if (!found) {
    return std::nullopt;
  }
  return best;
}

VoxelCoord voxel_cell_for_place(const MapData& map, Vec3 world_hit, int layer_y) {
  if (const auto face = pick_occupancy_face_at(map.occupancy, world_hit, map.tile_size);
      face.has_value()) {
    return adjacent_voxel(VoxelCoord{face->x, face->y, face->z}, face->face);
  }
  const TileCoord tile = world_to_tile_xz(world_hit, map.tile_size);
  return {tile.x, layer_y, tile.z};
}

VoxelCoord voxel_cell_for_remove(const MapData& map, Vec3 world_hit, int layer_y) {
  if (const auto face = pick_occupancy_face_at(map.occupancy, world_hit, map.tile_size);
      face.has_value()) {
    return {face->x, face->y, face->z};
  }
  const TileCoord tile = world_to_tile_xz(world_hit, map.tile_size);
  return {tile.x, layer_y, tile.z};
}

}  // namespace rat
