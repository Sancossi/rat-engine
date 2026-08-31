#include "rat/blocker_edit.hpp"

#include <algorithm>
#include <cmath>

namespace rat {
namespace {

float snap_value(float v, float tile) {
  if (tile <= 1e-6f) {
    return v;
  }
  return std::floor(v / tile + 0.5f) * tile;
}

}  // namespace

Aabb2 normalize_aabb(Aabb2 box) {
  if (box.min_x > box.max_x) {
    std::swap(box.min_x, box.max_x);
  }
  if (box.min_z > box.max_z) {
    std::swap(box.min_z, box.max_z);
  }
  return box;
}

Aabb2 snap_aabb_to_grid(Aabb2 box, float tile_size) {
  box = normalize_aabb(box);
  box.min_x = snap_value(box.min_x, tile_size);
  box.min_z = snap_value(box.min_z, tile_size);
  box.max_x = snap_value(box.max_x, tile_size);
  box.max_z = snap_value(box.max_z, tile_size);
  box = normalize_aabb(box);
  // Ensure at least one tile of extent after snap collapse.
  if (box.max_x - box.min_x < tile_size) {
    box.max_x = box.min_x + tile_size;
  }
  if (box.max_z - box.min_z < tile_size) {
    box.max_z = box.min_z + tile_size;
  }
  return box;
}

Aabb2 translate_aabb_on_grid(Aabb2 box, int tile_dx, int tile_dz, float tile_size) {
  const float dx = static_cast<float>(tile_dx) * tile_size;
  const float dz = static_cast<float>(tile_dz) * tile_size;
  box.min_x += dx;
  box.max_x += dx;
  box.min_z += dz;
  box.max_z += dz;
  return box;
}

Aabb2 resize_aabb_on_grid(Aabb2 box, AabbEdge edge, int tile_delta, float tile_size) {
  box = normalize_aabb(box);
  const float delta = static_cast<float>(tile_delta) * tile_size;
  switch (edge) {
    case AabbEdge::MinX: {
      float next = box.min_x + delta;
      if (next > box.max_x - tile_size) {
        next = box.max_x - tile_size;
      }
      box.min_x = next;
      break;
    }
    case AabbEdge::MaxX: {
      float next = box.max_x + delta;
      if (next < box.min_x + tile_size) {
        next = box.min_x + tile_size;
      }
      box.max_x = next;
      break;
    }
    case AabbEdge::MinZ: {
      float next = box.min_z + delta;
      if (next > box.max_z - tile_size) {
        next = box.max_z - tile_size;
      }
      box.min_z = next;
      break;
    }
    case AabbEdge::MaxZ: {
      float next = box.max_z + delta;
      if (next < box.min_z + tile_size) {
        next = box.min_z + tile_size;
      }
      box.max_z = next;
      break;
    }
  }
  return normalize_aabb(box);
}

}  // namespace rat
