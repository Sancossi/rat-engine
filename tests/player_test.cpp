#include <rat/player.hpp>
#include <rat/map_data.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using Catch::Approx;

namespace {

rat::MapData make_surface_map(int width, int height, std::vector<float> ground_y) {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "player_surface";
  map.width = width;
  map.height = height;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = width;
  map.height_grid.height = height;
  map.height_grid.ground_y = std::move(ground_y);
  return map;
}

}  // namespace

TEST_CASE("Player moves freely from WASD axes on XZ", "[unit][player]") {
  rat::PlayerBody player;
  player.x = 0.0f;
  player.z = 0.0f;
  player.speed = 4.0f;

  rat::MoveInput input;
  input.axis_x = 1.0f;
  input.axis_z = 0.0f;

  const auto blockers = std::vector<rat::BlockerDef>{};
  player = rat::integrate_player(player, input, 0.5f, blockers);
  REQUIRE(player.x == Approx(2.0f));
  REQUIRE(player.z == Approx(0.0f));
}

TEST_CASE("Player AABB slides along blockers instead of sticking", "[unit][player]") {
  rat::PlayerBody player;
  player.x = 0.0f;
  player.z = 0.0f;
  player.half_extent = 0.4f;
  player.speed = 5.0f;

  // Wall covering x>=1 for a strip in z.
  const std::vector<rat::BlockerDef> blockers = {
      rat::BlockerDef{.bounds = rat::Aabb2{1.0f, -2.0f, 3.0f, 2.0f}},
  };

  // Push into the wall on X while also moving on Z — X stops, Z slides.
  rat::MoveInput input;
  input.axis_x = 1.0f;
  input.axis_z = 1.0f;
  for (int i = 0; i < 30; ++i) {
    player = rat::integrate_player(player, input, 1.0f / 60.0f, blockers);
  }

  REQUIRE(player.x == Approx(0.6f).margin(0.05f));  // stopped before wall
  REQUIRE(player.z > 0.5f);                          // still slid on Z
  REQUIRE_FALSE(rat::aabb_overlap(
      rat::Aabb2{player.x - player.half_extent, player.z - player.half_extent,
                 player.x + player.half_extent, player.z + player.half_extent},
      blockers.front().bounds));
}

TEST_CASE("snap_to_grid is optional helper and does not force step move", "[unit][player]") {
  // Cell [1,2) x [3,4) → center (1.5, 3.5) for tile_size 1.
  const auto snapped = rat::snap_to_grid(1.2f, 0.0f, 3.7f, 1.0f);
  REQUIRE(snapped.x == Approx(1.5f));
  REQUIRE(snapped.z == Approx(3.5f));

  const auto origin_cell = rat::snap_to_grid(-0.1f, 0.0f, 0.2f, 1.0f);
  REQUIRE(origin_cell.x == Approx(-0.5f));
  REQUIRE(origin_cell.z == Approx(0.5f));
}

TEST_CASE("Camera-relative WASD aligns with ortho 3/4 view", "[unit][player]") {
  // Camera sits in +X+Z; look toward origin. W (screen forward) should move -X-Z.
  const rat::Vec3 eye{16.0f, 20.0f, 16.0f};
  const rat::Vec3 focus{0.0f, 0.0f, 0.0f};

  const auto forward = rat::camera_relative_move(0.0f, 1.0f, eye, focus);
  REQUIRE(forward.axis_x == Approx(-std::sqrt(0.5f)).margin(0.01f));
  REQUIRE(forward.axis_z == Approx(-std::sqrt(0.5f)).margin(0.01f));

  const auto right = rat::camera_relative_move(1.0f, 0.0f, eye, focus);
  REQUIRE(right.axis_x == Approx(-std::sqrt(0.5f)).margin(0.01f));
  REQUIRE(right.axis_z == Approx(std::sqrt(0.5f)).margin(0.01f));
}

TEST_CASE("Camera-relative WASD for top-down uses world XZ axes", "[unit][player]") {
  const rat::Vec3 eye{0.0f, 32.0f, 0.0f};
  const rat::Vec3 focus{0.0f, 0.0f, 0.0f};

  const auto forward = rat::camera_relative_move(0.0f, 1.0f, eye, focus);
  REQUIRE(forward.axis_x == Approx(0.0f));
  REQUIRE(forward.axis_z == Approx(-1.0f));

  // D (screen right) -> -X with top-down up=-Z / LH view.
  const auto right = rat::camera_relative_move(1.0f, 0.0f, eye, focus);
  REQUIRE(right.axis_x == Approx(-1.0f));
  REQUIRE(right.axis_z == Approx(0.0f));
}

TEST_CASE("World-aligned WASD stays stable across camera switches", "[unit][player]") {
  const auto forward = rat::world_aligned_move(0.0f, 1.0f);
  REQUIRE(forward.axis_x == Approx(0.0f));
  REQUIRE(forward.axis_z == Approx(-1.0f));

  const auto left = rat::world_aligned_move(-1.0f, 0.0f);
  REQUIRE(left.axis_x == Approx(1.0f));
  REQUIRE(left.axis_z == Approx(0.0f));

  const auto right = rat::world_aligned_move(1.0f, 0.0f);
  REQUIRE(right.axis_x == Approx(-1.0f));
  REQUIRE(right.axis_z == Approx(0.0f));
}

TEST_CASE("Ground-follow uses sampled flat tile height", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {0.25f, 0.25f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.25f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 2.0f;

  rat::MoveInput input;
  input.axis_x = 1.0f;
  input.axis_z = 0.0f;

  player = rat::integrate_player_surface(player, input, 0.1f, {}, query);
  REQUIRE(player.y == Approx(0.25f));
}

TEST_CASE("Ground-follow samples current position while idle", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(1, 1, {0.3f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 2.0f;
  player.z = 0.5f;

  player = rat::integrate_player_surface(player, rat::MoveInput{}, 0.1f, {}, query);
  REQUIRE(player.y == Approx(0.3f));
}

TEST_CASE("Ground-follow on ramp changes Y smoothly", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.05f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 1.0f;

  rat::MoveInput east{1.0f, 0.0f};
  float prev_y = player.y;
  for (int i = 0; i < 8; ++i) {
    player = rat::integrate_player_surface(player, east, 0.1f, {}, query);
    REQUIRE(player.y >= prev_y - 1e-5f);
    prev_y = player.y;
  }
  REQUIRE(player.y > 0.6f);

  rat::MoveInput west{-1.0f, 0.0f};
  for (int i = 0; i < 8; ++i) {
    const float before = player.y;
    player = rat::integrate_player_surface(player, west, 0.1f, {}, query);
    REQUIRE(player.y <= before + 1e-5f);
  }
  REQUIRE(player.y < 0.4f);
}

TEST_CASE("Too-high step-up blocks horizontal advance", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 1.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 4.0f;

  rat::MoveInput input{1.0f, 0.0f};
  for (int i = 0; i < 30; ++i) {
    player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f);
  }

  REQUIRE(player.x < 1.0f);
  REQUIRE(player.y == Approx(0.0f));
}

TEST_CASE("Ramp climb ignores flat-step cap and crests to high flat", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {2.0f, 2.0f});
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 2.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.05f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.half_extent = 0.8f;
  player.speed = 3.0f;

  rat::MoveInput input{1.0f, 0.0f};
  float prev_y = player.y;
  for (int i = 0; i < 24; ++i) {
    player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f);
    REQUIRE(player.y >= prev_y - 1e-4f);
    prev_y = player.y;
  }

  REQUIRE(player.x > 1.05f);
  REQUIRE(player.y == Approx(2.0f).margin(0.05f));
}

TEST_CASE("Diagonal move slides when X step-up is too high", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 2, {
      0.0f, 1.0f,
      0.0f, 1.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.95f;
  player.y = 0.0f;
  player.z = 0.2f;
  player.speed = 2.0f;

  rat::MoveInput diag{1.0f, 1.0f};
  player = rat::integrate_player_surface(player, diag, 0.2f, {}, query, 0.35f);

  REQUIRE(player.x == Approx(0.95f).margin(0.01f));
  REQUIRE(player.z > 0.2f);
  REQUIRE(player.y == Approx(0.0f));
}

TEST_CASE("Surface move keeps X then Z semantics", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 2, {
      0.0f, 1.0f,
      0.0f, 0.0f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.95f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 1.0f;
  player.half_extent = 0.4f;

  // One substep: X checks high tile first and blocks, then Z advances.
  rat::MoveInput diag{1.0f, 1.0f};
  player = rat::integrate_player_surface(player, diag, 0.2f, {}, query, 0.35f);

  REQUIRE(player.x == Approx(0.95f).margin(0.01f));
  REQUIRE(player.z > 0.5f);
}

TEST_CASE("Legacy integrate_player flat movement remains", "[unit][player]") {
  rat::PlayerBody player;
  player.x = 0.0f;
  player.z = 0.0f;
  player.speed = 4.0f;

  rat::MoveInput input;
  input.axis_x = 1.0f;
  input.axis_z = 0.0f;

  player = rat::integrate_player(player, input, 0.5f, {});
  REQUIRE(player.x == Approx(2.0f));
  REQUIRE(player.z == Approx(0.0f));
}
