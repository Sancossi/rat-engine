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
