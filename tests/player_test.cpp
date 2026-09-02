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

rat::MapData make_east_ramp_side_entry_map() {
  constexpr int kWidth = 10;
  constexpr int kHeight = 10;
  std::vector<float> ground(static_cast<std::size_t>(kWidth * kHeight), 0.0f);

  // High platform connected to the ramp's east edge.
  ground[8 * kWidth + 9] = 1.0f;

  rat::MapData map = make_surface_map(kWidth, kHeight, std::move(ground));
  map.ramps.push_back({
      .tile = rat::TileCoord{8, 8},
      .direction = rat::RampDirection::East,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });
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

TEST_CASE("Ramp side entry from north and south is blocked by step cap", "[unit][player][surface]") {
  const rat::MapData map = make_east_ramp_side_entry_map();
  const rat::SurfaceQuery query(map);
  constexpr float kMaxStepUp = 0.35f;

  auto attempt = [&](float start_x, float start_z, float axis_x, float axis_z) {
    rat::PlayerBody player;
    player.x = start_x;
    player.z = start_z;
    player.y = query.sample(start_x, start_z).y;
    player.speed = 1.0f;

    player = rat::integrate_player_surface(player, rat::MoveInput{axis_x, axis_z}, 0.2f, {}, query,
                                           kMaxStepUp, {}, &map);
    return player;
  };

  const rat::PlayerBody from_north = attempt(8.5f, 7.95f, 0.0f, 1.0f);
  REQUIRE(from_north.z < 8.0f);
  REQUIRE(from_north.y <= Approx(kMaxStepUp).margin(1e-4f));

  const rat::PlayerBody from_south = attempt(8.5f, 9.05f, 0.0f, -1.0f);
  REQUIRE(from_south.z >= 9.0f);
  REQUIRE(from_south.y <= Approx(kMaxStepUp).margin(1e-4f));
}

TEST_CASE("Ramp open-side walk-off with baked walls leaves the footprint",
          "[unit][player][surface]") {
  const rat::MapData map = make_east_ramp_side_entry_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 8.5f;
  player.z = 8.5f;
  player.y = query.sample(player.x, player.z).y;
  player.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.move = {0.0f, -1.0f};

  const float start_y = player.y;
  REQUIRE(query.sample(player.x, player.z).on_ramp);
  for (int i = 0; i < 60; ++i) {
    const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
        player, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
    player = result.body;
    jump = result.jump;
    if (!query.sample(player.x, player.z).on_ramp) {
      break;
    }
  }

  REQUIRE(player.z < 8.0f);
  REQUIRE_FALSE(query.sample(player.x, player.z).on_ramp);
  REQUIRE_FALSE(jump.grounded);
  REQUIRE(player.y == Approx(start_y).margin(0.2f));
}

TEST_CASE("Ramp west approach climbs to high cell with baked terrain walls",
          "[unit][player][surface]") {
  const rat::MapData map = make_east_ramp_side_entry_map();
  const rat::SurfaceQuery query(map);

  auto climb_from = [&](float start_z) {
    rat::PlayerBody player;
    player.x = 7.5f;
    player.z = start_z;
    player.y = query.sample(player.x, player.z).y;
    player.speed = 5.0f;
    rat::JumpState jump = rat::make_grounded_jump_state();
    rat::PlayerFrameInput input;
    input.move = {1.0f, 0.0f};
    for (int i = 0; i < 200; ++i) {
      const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
          player, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
      player = result.body;
      jump = result.jump;
      if (player.x > 9.05f && player.x < 10.0f && player.y > 0.5f) {
        break;
      }
    }
    return player;
  };

  const rat::PlayerBody center = climb_from(8.5f);
  REQUIRE(center.x > 9.0f);
  REQUIRE(center.y == Approx(1.0f).margin(0.06f));
  REQUIRE(center.z == Approx(8.5f).margin(1e-4f));

  // Still the west (low) approach — not north/south side entry — just off tile center.
  const rat::PlayerBody off_center = climb_from(8.3f);
  REQUIRE(off_center.x > 9.0f);
  REQUIRE(off_center.y == Approx(1.0f).margin(0.06f));
  REQUIRE(off_center.z == Approx(8.3f).margin(1e-4f));
}

TEST_CASE("Ramp low-edge and low-side diagonals remain allowed", "[unit][player][surface]") {
  const rat::MapData map = make_east_ramp_side_entry_map();
  const rat::SurfaceQuery query(map);

  auto step_once = [&](float start_x, float start_z, float axis_x, float axis_z) {
    rat::PlayerBody player;
    player.x = start_x;
    player.z = start_z;
    player.y = query.sample(start_x, start_z).y;
    player.speed = 1.0f;
    return rat::integrate_player_surface(player, rat::MoveInput{axis_x, axis_z}, 0.2f, {}, query,
                                           0.35f, {}, &map);
  };

  const rat::PlayerBody west_entry = step_once(7.95f, 8.5f, 1.0f, 0.0f);
  REQUIRE(west_entry.x > 8.0f);
  REQUIRE(west_entry.y > 0.0f);
  REQUIRE(west_entry.y <= 0.35f);

  const rat::PlayerBody from_nw = step_once(7.95f, 7.95f, 1.0f, 1.0f);
  REQUIRE(from_nw.x > 8.0f);
  REQUIRE(from_nw.z > 8.0f);
  REQUIRE(from_nw.y > 0.0f);
  REQUIRE(from_nw.y <= 0.35f);

  const rat::PlayerBody from_sw = step_once(7.95f, 9.05f, 1.0f, -1.0f);
  REQUIRE(from_sw.x > 8.0f);
  REQUIRE(from_sw.z < 9.0f);
  REQUIRE(from_sw.y > 0.0f);
  REQUIRE(from_sw.y <= 0.35f);
}

TEST_CASE("Ramp high-edge descent and same-ramp steep rise stay allowed",
          "[unit][player][surface]") {
  const rat::MapData map = make_east_ramp_side_entry_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody from_platform;
  from_platform.x = 9.05f;
  from_platform.z = 8.5f;
  from_platform.y = query.sample(from_platform.x, from_platform.z).y;
  from_platform.speed = 1.0f;
  from_platform = rat::integrate_player_surface(from_platform, rat::MoveInput{-1.0f, 0.0f}, 0.2f, {}, query,
                                                0.35f);
  REQUIRE(from_platform.x < 9.0f);
  REQUIRE(from_platform.y < 1.0f);
  REQUIRE(from_platform.y > 0.5f);

  rat::PlayerBody on_ramp;
  on_ramp.x = 8.2f;
  on_ramp.z = 8.5f;
  on_ramp.y = query.sample(on_ramp.x, on_ramp.z).y;
  on_ramp.speed = 1.0f;

  float prev_y = on_ramp.y;
  for (int i = 0; i < 4; ++i) {
    on_ramp = rat::integrate_player_surface(on_ramp, rat::MoveInput{1.0f, 0.0f}, 0.1f, {}, query, 0.05f);
    REQUIRE(on_ramp.y >= prev_y - 1e-5f);
    prev_y = on_ramp.y;
  }
  REQUIRE(on_ramp.x > 8.5f);
  REQUIRE(on_ramp.y > 0.5f);
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

TEST_CASE("Walk into a 0.45 east fence blocks X like a too-high step-up",
          "[unit][player][surface][edge]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
  map.edge_barriers.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .height = 0.45f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 4.0f;

  rat::MoveInput input{1.0f, 0.0f};
  for (int i = 0; i < 30; ++i) {
    player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f,
                                           map.edge_barriers);
  }

  REQUIRE(player.x < 1.0f);
  REQUIRE(player.y == Approx(0.0f));
}

TEST_CASE("Walk along adjacent tile into east fence blocks Z", "[unit][player][edge]") {
  rat::MapData map = make_surface_map(2, 2, {0.0f, 0.0f, 0.0f, 0.0f});
  map.edge_barriers.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .height = 0.45f,
  });
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 1.3f;
  player.y = 0.0f;
  player.z = -0.5f;
  player.half_extent = 0.4f;
  player.speed = 4.0f;

  rat::MoveInput input{0.0f, 1.0f};
  for (int i = 0; i < 60; ++i) {
    player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f,
                                           map.edge_barriers);
  }

  REQUIRE(player.z < 0.0f);
}

TEST_CASE("Side-walk along a 1.0 cube west face blocks Z", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 2, {0.0f, 1.0f, 0.0f, 0.0f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.7f;
  player.y = 0.0f;
  player.z = -0.3f;
  player.half_extent = 0.4f;
  player.speed = 4.0f;

  rat::MoveInput input{0.0f, 1.0f};
  for (int i = 0; i < 60; ++i) {
    player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  }

  REQUIRE(player.z < 0.0f);
}

TEST_CASE("Stair 0.25 still walks up when map walls are baked", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.25f});
  const rat::SurfaceQuery query(map);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 4.0f;

  rat::MoveInput input{1.0f, 0.0f};
  for (int i = 0; i < 18; ++i) {
    player = rat::integrate_player_surface(player, input, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  }

  REQUIRE(player.x > 1.0f);
  REQUIRE(player.x < 2.0f);
  REQUIRE(player.y == Approx(0.25f));
}

TEST_CASE("Player stands on airborne slab and falls walking off", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody player;
  player.x = 0.5f; player.y = 2.0f; player.z = 0.5f; player.speed = 5.0f;
  player = rat::integrate_player_surface(player, {}, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  REQUIRE(player.y == Approx(2.0f).margin(1e-3f));
  rat::MoveInput east{1.0f, 0.0f};
  for (int i = 0; i < 40; ++i) {
    player = rat::integrate_player_surface(player, east, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  }
  REQUIRE(player.x > 1.2f);
  REQUIRE(player.y == Approx(0.0f).margin(0.05f));
}

TEST_CASE("East ladder climb reaches slab with interact then into-face", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody player;
  player.x = 0.85f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  input.move = {1.0f, 0.0f};
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      player, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  player = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  for (int i = 0; i < 80; ++i) {
    result = rat::integrate_player_frame_surface(player, jump, input, 1.0f / 60.0f, {}, query, {},
                                                 0.35f, {}, &map);
    player = result.body;
    jump = result.jump;
    if (!result.jump.climbing) {
      break;
    }
  }
  REQUIRE(player.y == Approx(2.0f).margin(0.1f));
}

TEST_CASE("Neighboring ramps with a rise above max_step_up cannot be climbed",
          "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
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
  const rat::SurfaceQuery query(map);
  constexpr float kMaxStepUp = 0.35f;

  rat::PlayerBody player;
  player.x = 0.85f;
  player.z = 0.5f;
  player.y = query.sample(player.x, player.z).y;
  player.speed = 2.0f;

  const rat::SurfaceSample from = query.sample(player.x, player.z);
  const rat::SurfaceSample to = query.sample(1.1f, 0.5f);
  REQUIRE(from.on_ramp);
  REQUIRE(to.on_ramp);
  REQUIRE(from.ramp_index != to.ramp_index);
  REQUIRE(to.y - from.y > kMaxStepUp);

  player = rat::integrate_player_surface(player, rat::MoveInput{1.0f, 0.0f}, 0.2f, {}, query,
                                         kMaxStepUp, {}, &map);
  REQUIRE(player.x < 1.0f);
  REQUIRE(player.y < 0.8f);
}

TEST_CASE("East ladder climb uses camera steer not world WASD", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody player;
  player.x = 0.85f;
  player.y = 0.0f;
  player.z = 0.5f;
  player.speed = 5.0f;
  const rat::ClimbCameraPose pose =
      rat::climb_camera_pose(rat::Vec3{player.x, player.y, player.z}, 1.0f, 0.0f);
  const rat::MoveInput world_w = rat::world_aligned_move(0.0f, 1.0f);
  const rat::MoveInput cam_w = rat::camera_relative_move(0.0f, 1.0f, pose.eye, pose.focus);
  REQUIRE(world_w.axis_z == Approx(-1.0f));
  REQUIRE(cam_w.axis_x > 0.5f);
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.interact_pressed = true;
  rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      player, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.climbing);
  player = result.body;
  jump = result.jump;
  input.interact_pressed = false;
  input.move = world_w;
  input.climb_move = cam_w;
  for (int i = 0; i < 80; ++i) {
    result = rat::integrate_player_frame_surface(player, jump, input, 1.0f / 60.0f, {}, query, {},
                                                 0.35f, {}, &map);
    player = result.body;
    jump = result.jump;
    if (player.y >= 1.9f) {
      break;
    }
  }
  REQUIRE(player.y == Approx(2.0f).margin(0.1f));
}
