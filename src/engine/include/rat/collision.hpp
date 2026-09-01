#pragma once

#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/surface_query.hpp"

#include <span>
#include <vector>

namespace rat {

constexpr float kPlayerCylinderHeight = 1.6f;
constexpr float kFenceFeetClearanceEpsilon = 1e-4f;

struct FenceSolid {
  float ax = 0.0f;
  float az = 0.0f;
  float bx = 0.0f;
  float bz = 0.0f;
  float y_lo = 0.0f;
  float y_hi = 0.0f;
};

struct CollisionBody {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float radius = 0.4f;
  float height = kPlayerCylinderHeight;
  float mass = 1.0f;
  float vel_x = 0.0f;
  float vel_y = 0.0f;
  float vel_z = 0.0f;
};

struct CollisionWorld {
  std::vector<FenceSolid> fences;
};

[[nodiscard]] CollisionBody collision_body_from_player(const PlayerBody& player);
[[nodiscard]] CollisionWorld bake_fence_world(std::span<const EdgeBarrierDef> barriers,
                                              const SurfaceQuery& query);
[[nodiscard]] bool cylinder_hits_fences(const CollisionBody& body, const CollisionWorld& world);
[[nodiscard]] bool circle_overlaps_aabb2(float cx, float cz, float radius, const Aabb2& box);

}  // namespace rat
