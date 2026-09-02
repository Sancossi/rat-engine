#include <rat/collision.hpp>
#include <rat/map_data.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

using Catch::Approx;

namespace {

rat::MapData make_grid(int w, int h, float ground = 0.0f) {
  rat::MapData map;
  map.schema_version = 2;
  map.tile_size = 1.0f;
  map.width = w;
  map.height = h;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = w;
  map.height_grid.height = h;
  map.height_grid.ground_y.assign(static_cast<std::size_t>(w * h), ground);
  return map;
}

}  // namespace

TEST_CASE("Bake east fence on tile (0,0) is a vertical segment at x=1", "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef barrier;
  barrier.tile = {0, 0};
  barrier.direction = rat::RampDirection::East;
  barrier.height = 0.45f;

  const rat::CollisionWorld world = rat::bake_fence_world(std::span<const rat::EdgeBarrierDef>(&barrier, 1), query);

  REQUIRE(world.fences.size() == 1);
  const rat::FenceSolid& fence = world.fences[0];
  REQUIRE(fence.ax == Approx(1.0f));
  REQUIRE(fence.bx == Approx(1.0f));
  REQUIRE(std::min(fence.az, fence.bz) == Approx(0.0f));
  REQUIRE(std::max(fence.az, fence.bz) == Approx(1.0f));
  REQUIRE(fence.y_lo == Approx(0.0f));
  REQUIRE(fence.y_hi == Approx(0.45f));
}

TEST_CASE("collision_body_from_player copies feet and uses half_extent as radius", "[collision]") {
  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.half_extent = 0.4f;

  const rat::CollisionBody body = rat::collision_body_from_player(player);

  REQUIRE(body.x == Approx(0.5f));
  REQUIRE(body.y == Approx(0.0f));
  REQUIRE(body.z == Approx(0.5f));
  REQUIRE(body.radius == Approx(0.4f));
  REQUIRE(body.height == Approx(1.6f));
}

TEST_CASE("Cylinder 0.4 from tile center misses east fence; 0.3 away hits", "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef barrier;
  barrier.tile = {0, 0};
  barrier.direction = rat::RampDirection::East;
  barrier.height = 0.45f;

  const rat::CollisionWorld world = rat::bake_fence_world(std::span<const rat::EdgeBarrierDef>(&barrier, 1), query);

  rat::CollisionBody miss;
  miss.x = 0.5f;
  miss.y = 0.0f;
  miss.z = 0.5f;
  miss.radius = 0.4f;
  miss.height = rat::kPlayerCylinderHeight;
  REQUIRE_FALSE(rat::cylinder_hits_fences(miss, world));

  rat::CollisionBody hit = miss;
  hit.x = 0.7f;
  REQUIRE(rat::cylinder_hits_fences(hit, world));
}

TEST_CASE("Cylinder on adjacent tile hits east fence without crossing x=1", "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef barrier;
  barrier.tile = {0, 0};
  barrier.direction = rat::RampDirection::East;
  barrier.height = 0.45f;

  const rat::CollisionWorld world = rat::bake_fence_world(std::span<const rat::EdgeBarrierDef>(&barrier, 1), query);

  rat::CollisionBody body;
  body.x = 1.3f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.radius = 0.4f;
  body.height = rat::kPlayerCylinderHeight;

  REQUIRE(body.x > 1.0f);
  REQUIRE(rat::cylinder_hits_fences(body, world));
}

TEST_CASE("Feet at Mini fence top do not hit", "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef barrier;
  barrier.tile = {0, 0};
  barrier.direction = rat::RampDirection::East;
  barrier.height = 0.45f;

  const rat::CollisionWorld world = rat::bake_fence_world(std::span<const rat::EdgeBarrierDef>(&barrier, 1), query);

  rat::CollisionBody body;
  body.x = 0.7f;
  body.y = 0.45f;
  body.z = 0.5f;
  body.radius = 0.4f;
  body.height = rat::kPlayerCylinderHeight;

  REQUIRE_FALSE(rat::cylinder_hits_fences(body, world));
}

TEST_CASE("Bake skips height <= 0 barriers", "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef zero;
  zero.tile = {0, 0};
  zero.direction = rat::RampDirection::East;
  zero.height = 0.0f;

  rat::EdgeBarrierDef negative = zero;
  negative.height = -0.45f;

  const rat::EdgeBarrierDef barriers[] = {zero, negative};
  const rat::CollisionWorld world = rat::bake_fence_world(barriers, query);

  REQUIRE(world.fences.empty());
}

TEST_CASE("Bake 2x1 ground 0 vs 1 has a wall on x=1 with span 1", "[collision]") {
  rat::MapData map = make_grid(2, 1);
  map.height_grid.ground_y = {0.0f, 1.0f};
  const rat::SurfaceQuery query(map);

  const rat::CollisionWorld world = rat::bake_collision_world(map, query);

  bool found = false;
  for (const rat::FenceSolid& solid : world.fences) {
    if (solid.ax == Approx(1.0f) && solid.bx == Approx(1.0f) &&
        (solid.y_hi - solid.y_lo) == Approx(1.0f)) {
      found = true;
      break;
    }
  }
  REQUIRE(found);
}

TEST_CASE("Cylinder hits 1.0 terrain wall at feet 0 and misses at feet 1", "[collision]") {
  rat::MapData map = make_grid(2, 1);
  map.height_grid.ground_y = {0.0f, 1.0f};
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);

  rat::CollisionBody at_low;
  at_low.x = 0.7f;
  at_low.y = 0.0f;
  at_low.z = 0.5f;
  at_low.radius = 0.4f;
  at_low.height = rat::kPlayerCylinderHeight;
  REQUIRE(rat::cylinder_hits_walls(at_low, world, 0.35f));

  rat::CollisionBody at_high = at_low;
  at_high.y = 1.0f;
  REQUIRE_FALSE(rat::cylinder_hits_walls(at_high, world, 0.35f));
}

TEST_CASE("Cylinder does not hit 0.25 stair wall with max_step_up 0.35", "[collision]") {
  rat::MapData map = make_grid(2, 1);
  map.height_grid.ground_y = {0.0f, 0.25f};
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);

  rat::CollisionBody body;
  body.x = 0.7f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.radius = 0.4f;
  body.height = rat::kPlayerCylinderHeight;
  REQUIRE_FALSE(rat::cylinder_hits_walls(body, world, 0.35f));
}

TEST_CASE("East Mini 0.45 fence still blocks at feet 0 with cylinder_hits_walls 0.35",
          "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef barrier;
  barrier.tile = {0, 0};
  barrier.direction = rat::RampDirection::East;
  barrier.height = 0.45f;

  const rat::CollisionWorld world =
      rat::bake_fence_world(std::span<const rat::EdgeBarrierDef>(&barrier, 1), query);

  rat::CollisionBody miss;
  miss.x = 0.5f;
  miss.y = 0.0f;
  miss.z = 0.5f;
  miss.radius = 0.4f;
  miss.height = rat::kPlayerCylinderHeight;
  REQUIRE_FALSE(rat::cylinder_hits_walls(miss, world, 0.35f));

  rat::CollisionBody hit = miss;
  hit.x = 0.7f;
  REQUIRE(rat::cylinder_hits_walls(hit, world, 0.35f));

  rat::CollisionBody adjacent = miss;
  adjacent.x = 1.3f;
  REQUIRE(rat::cylinder_hits_walls(adjacent, world, 0.35f));

  rat::CollisionBody on_top = hit;
  on_top.y = 0.45f;
  REQUIRE_FALSE(rat::cylinder_hits_walls(on_top, world, 0.35f));
}

TEST_CASE("East Mini 0.45 fence still blocks when feet are slightly above 0", "[collision]") {
  const rat::MapData map = make_grid(2, 2);
  const rat::SurfaceQuery query(map);

  rat::EdgeBarrierDef barrier;
  barrier.tile = {0, 0};
  barrier.direction = rat::RampDirection::East;
  barrier.height = 0.45f;

  const rat::CollisionWorld world =
      rat::bake_fence_world(std::span<const rat::EdgeBarrierDef>(&barrier, 1), query);

  rat::CollisionBody airborne;
  airborne.x = 0.7f;
  airborne.y = 0.10f;
  airborne.z = 0.5f;
  airborne.radius = 0.4f;
  airborne.height = rat::kPlayerCylinderHeight;
  REQUIRE(rat::cylinder_hits_walls(airborne, world, 0.35f));

  rat::CollisionBody adjacent;
  adjacent.x = 1.3f;
  adjacent.y = 0.10f;
  adjacent.z = 0.5f;
  adjacent.radius = 0.4f;
  adjacent.height = rat::kPlayerCylinderHeight;
  REQUIRE(rat::cylinder_hits_walls(adjacent, world, 0.35f));
}

TEST_CASE("Off-center west approach misses ramp side where local height is under max_step_up",
          "[collision]") {
  rat::MapData map = make_grid(4, 3);
  map.height_grid.ground_y[1 * 4 + 2] = 1.0f;
  map.ramps.push_back({
      .tile = rat::TileCoord{1, 1},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);

  rat::CollisionBody low_west;
  low_west.x = 1.2f;
  low_west.y = 0.0f;
  low_west.z = 1.3f;
  low_west.radius = 0.4f;
  low_west.height = rat::kPlayerCylinderHeight;
  REQUIRE_FALSE(rat::cylinder_hits_walls(low_west, world, 0.35f));

  rat::CollisionBody high_side;
  high_side.x = 1.7f;
  high_side.y = 0.0f;
  high_side.z = 1.15f;
  high_side.radius = 0.4f;
  high_side.height = rat::kPlayerCylinderHeight;
  REQUIRE(rat::cylinder_hits_walls(high_side, world, 0.35f));
}

TEST_CASE("circle_overlaps_aabb2 misses square corner and hits axis within radius", "[collision]") {
  const rat::Aabb2 box{0.0f, 0.0f, 1.0f, 1.0f};
  constexpr float kRadius = 0.4f;

  REQUIRE_FALSE(rat::circle_overlaps_aabb2(1.35f, 1.35f, kRadius, box));
  REQUIRE(rat::circle_overlaps_aabb2(1.3f, 0.5f, kRadius, box));
}

TEST_CASE("Bake ground box for a 1.0 cell and skip ramp tiles", "[collision]") {
  rat::MapData map = make_grid(1, 1, 1.0f);
  const rat::SurfaceQuery query(map);
  rat::CollisionWorld world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.size() == 1);
  REQUIRE(world.boxes[0].y_hi == Approx(1.0f));
  REQUIRE(world.boxes[0].y_lo == Approx(0.0f));

  map.ramps.push_back({.tile = {0, 0}, .direction = rat::RampDirection::East, .low_y = 0.0f, .high_y = 1.0f});
  world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.empty());  // prism comes in Task 3
}

TEST_CASE("West ramp prism interpolates like SurfaceQuery and replaces the ground box",
          "[collision]") {
  rat::MapData map = make_grid(1, 1, 0.0f);
  map.ramps.push_back({.tile = {0, 0},
                       .direction = rat::RampDirection::West,
                       .low_y = 0.0f,
                       .high_y = 1.0f});
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.empty());
  REQUIRE(world.ramps.size() == 1);
  REQUIRE(rat::ramp_surface_y(world.ramps[0], 0.0f, 0.5f) == Approx(query.sample(0.0f, 0.5f).y).margin(1e-4f));
  REQUIRE(rat::ramp_surface_y(world.ramps[0], 1.0f, 0.5f) == Approx(query.sample(1.0f, 0.5f).y).margin(1e-4f));
}

TEST_CASE("Bake floor slab is a thin box not filled to Y=0", "[collision]") {
  rat::MapData map = make_grid(1, 1, 0.0f);
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.size() == 2);
  bool found_slab = false;
  for (const rat::WalkableBox& box : world.boxes) {
    if (box.y_hi == Approx(2.0f) && box.y_lo == Approx(1.75f)) {
      found_slab = true;
    }
  }
  REQUIRE(found_slab);
}

TEST_CASE("query_solid_support reports distinct ramp indices", "[collision]") {
  rat::MapData map = make_grid(2, 1, 0.0f);
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 0.1f,
  });
  map.ramps.push_back({
      .tile = rat::TileCoord{1, 0},
      .direction = rat::RampDirection::East,
      .low_y = 0.8f,
      .high_y = 1.0f,
  });
  const rat::CollisionWorld world = rat::bake_collision_world(map, rat::SurfaceQuery(map));
  const auto first = rat::query_solid_support(world, 0.85f, 0.5f, 0.4f, 0.0f, 1.0e6f);
  const auto second = rat::query_solid_support(world, 1.1f, 0.5f, 0.4f, 0.8f, 1.0e6f);
  REQUIRE(first.has_value());
  REQUIRE(second.has_value());
  REQUIRE(first->on_ramp);
  REQUIRE(second->on_ramp);
  REQUIRE(first->ramp_index >= 0);
  REQUIRE(second->ramp_index >= 0);
  REQUIRE(first->ramp_index != second->ramp_index);
}

TEST_CASE("Cylinder head hits slab underside; feet on top do not", "[collision]") {
  rat::MapData map = make_grid(1, 1, 0.0f);
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::CollisionWorld world = rat::bake_collision_world(map, rat::SurfaceQuery(map));
  rat::CollisionBody under;
  under.x = 0.5f; under.y = 0.0f; under.z = 0.5f;
  under.height = 1.6f;
  REQUIRE(rat::cylinder_hits_ceiling(under, world));
  rat::CollisionBody on_top = under;
  on_top.y = 2.0f;
  REQUIRE_FALSE(rat::cylinder_hits_ceiling(on_top, world));
}

TEST_CASE("Cylinder in the hole beside a slab is not under that slab ceiling", "[collision]") {
  rat::MapData map = make_grid(2, 1, 0.0f);
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::CollisionWorld world = rat::bake_collision_world(map, rat::SurfaceQuery(map));
  rat::CollisionBody beside;
  beside.x = 1.04f;
  beside.y = 1.5f;
  beside.z = 0.5f;
  beside.height = 1.6f;
  REQUIRE_FALSE(rat::cylinder_hits_ceiling(beside, world));
}

TEST_CASE("Thin slab side blocks at mid-thickness even though span < max_step_up", "[collision]") {
  rat::MapData map = make_grid(2, 1, 0.0f);
  map.floor_slabs.push_back({{1, 0}, 2.0f, 0.25f});
  const rat::CollisionWorld world = rat::bake_collision_world(map, rat::SurfaceQuery(map));
  rat::CollisionBody body;
  body.x = 0.7f; body.y = 1.8f; body.z = 0.5f;
  REQUIRE(rat::cylinder_hits_walls(body, world, 0.35f));
}

TEST_CASE("Ladder face into maps east to +X", "[collision]") {
  float into_x = 0.0f;
  float into_z = 0.0f;
  rat::ladder_face_into(rat::RampDirection::East, into_x, into_z);
  REQUIRE(into_x == Approx(1.0f));
  REQUIRE(into_z == Approx(0.0f));
}
