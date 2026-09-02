#pragma once

#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/surface_query.hpp"

#include <optional>
#include <span>
#include <vector>

namespace rat {

constexpr float kPlayerCylinderHeight = 1.6f;
constexpr float kFenceFeetClearanceEpsilon = 1e-4f;
inline constexpr float kLadderInset = 0.3f;

struct FenceSolid {
  float ax = 0.0f;
  float az = 0.0f;
  float bx = 0.0f;
  float bz = 0.0f;
  float y_lo = 0.0f;
  float y_hi = 0.0f;
  float ay_lo = 0.0f;
  float ay_hi = 0.0f;
  float by_lo = 0.0f;
  float by_hi = 0.0f;
  bool apply_max_step_up_skip = true;
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

struct WalkableBox {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
  float y_lo = 0.0f;
  float y_hi = 0.0f;  // standable top; underside is ceiling
};

struct WalkableRamp {
  TileCoord tile;
  RampDirection direction = RampDirection::East;
  float low_y = 0.0f;
  float high_y = 0.0f;
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
  int ramp_index = -1;
};

struct LadderVolume {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
  float y_lo = 0.0f;
  float y_hi = 0.0f;
  RampDirection face = RampDirection::East;
};

struct CollisionWorld {
  std::vector<FenceSolid> fences;
  std::vector<WalkableBox> boxes;
  std::vector<WalkableRamp> ramps;
  std::vector<LadderVolume> ladders;
};

[[nodiscard]] CollisionBody collision_body_from_player(const PlayerBody& player);
[[nodiscard]] CollisionWorld bake_fence_world(std::span<const EdgeBarrierDef> barriers,
                                              const SurfaceQuery& query);
void append_terrain_walls(CollisionWorld& world, const HeightGrid& grid,
                          std::span<const RampDef> ramps, float tile_size);
void append_ground_boxes(CollisionWorld& world, const HeightGrid& grid,
                         std::span<const RampDef> ramps, float tile_size);
void append_floor_slabs(CollisionWorld& world, std::span<const FloorSlabDef> slabs, float tile_size);
[[nodiscard]] float ramp_surface_y(const WalkableRamp& ramp, float x, float z);
void append_ramp_prisms(CollisionWorld& world, std::span<const RampDef> ramps, float tile_size);
void append_ladders(CollisionWorld& world, std::span<const LadderDef> ladders, float tile_size);
[[nodiscard]] const LadderVolume* overlapping_ladder(const CollisionBody& body,
                                                     const CollisionWorld& world);
[[nodiscard]] CollisionWorld bake_collision_world(const MapData& map, const SurfaceQuery& query);

struct SolidSupport {
  float y = 0.0f;
  bool on_ramp = false;
  int ramp_index = -1;
};

[[nodiscard]] std::optional<SolidSupport> query_solid_support(
    const CollisionWorld& world, float x, float z, float radius, float feet_y,
    float max_step_up);
[[nodiscard]] bool cylinder_hits_ceiling(const CollisionBody& body, const CollisionWorld& world);

[[nodiscard]] bool cylinder_hits_fences(const CollisionBody& body, const CollisionWorld& world);
[[nodiscard]] bool cylinder_hits_walls(const CollisionBody& body, const CollisionWorld& world,
                                       float max_step_up = 0.35f);
void depenetrate_cylinder_from_walls(CollisionBody& body, const CollisionWorld& world,
                                     float max_step_up = 0.35f);
[[nodiscard]] bool circle_overlaps_aabb2(float cx, float cz, float radius, const Aabb2& box);

void ladder_face_into(RampDirection face, float& into_x, float& into_z);

}  // namespace rat
