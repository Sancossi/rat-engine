#pragma once

#include "rat/camera.hpp"

#include <span>

namespace rat {

struct Aabb2 {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
};

struct PlayerBody {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float half_extent = 0.4f;
  float speed = 5.0f;
};

struct MoveInput {
  float axis_x = 0.0f;  // A/D, world X
  float axis_z = 0.0f;  // W/S, world Z (W = -Z)
};

[[nodiscard]] PlayerBody integrate_player(PlayerBody player, MoveInput input, float dt,
                                          std::span<const Aabb2> blockers);

[[nodiscard]] Vec3 snap_to_grid(float x, float y, float z, float tile_size);

[[nodiscard]] bool aabb_overlap(const Aabb2& a, const Aabb2& b);

}  // namespace rat
