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
  float axis_x = 0.0f;  // world X delta weight (after camera remap)
  float axis_z = 0.0f;  // world Z delta weight (after camera remap)
};

// Screen WASD → world XZ for ortho 3/4: screen_x = A/D (-1..1), screen_z = S/W (-1..1, W=+1).
[[nodiscard]] MoveInput camera_relative_move(float screen_x, float screen_z, Vec3 eye,
                                             Vec3 focus);

[[nodiscard]] PlayerBody integrate_player(PlayerBody player, MoveInput input, float dt,
                                          std::span<const Aabb2> blockers);

[[nodiscard]] Vec3 snap_to_grid(float x, float y, float z, float tile_size);

[[nodiscard]] bool aabb_overlap(const Aabb2& a, const Aabb2& b);

}  // namespace rat
