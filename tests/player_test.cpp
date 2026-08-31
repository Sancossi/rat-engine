#include <rat/player.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using Catch::Approx;

TEST_CASE("Player moves freely from WASD axes on XZ", "[unit][player]") {
  rat::PlayerBody player;
  player.x = 0.0f;
  player.z = 0.0f;
  player.speed = 4.0f;

  rat::MoveInput input;
  input.axis_x = 1.0f;
  input.axis_z = 0.0f;

  const auto blockers = std::vector<rat::Aabb2>{};
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
  const std::vector<rat::Aabb2> blockers = {
      rat::Aabb2{1.0f, -2.0f, 3.0f, 2.0f},
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
      blockers.front()));
}

TEST_CASE("snap_to_grid is optional helper and does not force step move", "[unit][player]") {
  const auto snapped = rat::snap_to_grid(1.2f, 0.0f, 3.7f, 1.0f);
  REQUIRE(snapped.x == Approx(1.0f));
  REQUIRE(snapped.z == Approx(4.0f));
}
