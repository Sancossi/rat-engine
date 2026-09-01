#include <rat/blocker_edit.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("normalize_aabb swaps inverted corners", "[unit][blocker_edit]") {
  const auto box = rat::normalize_aabb({2.0f, 3.0f, 0.0f, 1.0f});
  REQUIRE(box.min_x == Approx(0.0f));
  REQUIRE(box.max_x == Approx(2.0f));
  REQUIRE(box.min_z == Approx(1.0f));
  REQUIRE(box.max_z == Approx(3.0f));
}

TEST_CASE("translate_aabb_on_grid moves by tile steps", "[unit][blocker_edit]") {
  const rat::Aabb2 box{3.0f, -1.0f, 5.0f, 1.0f};
  const auto moved = rat::translate_aabb_on_grid(box, 1, -2, 1.0f);
  REQUIRE(moved.min_x == Approx(4.0f));
  REQUIRE(moved.max_x == Approx(6.0f));
  REQUIRE(moved.min_z == Approx(-3.0f));
  REQUIRE(moved.max_z == Approx(-1.0f));
}

TEST_CASE("resize_aabb_on_grid grows max edge by tiles", "[unit][blocker_edit]") {
  const rat::Aabb2 box{0.0f, 0.0f, 1.0f, 1.0f};
  const auto grown = rat::resize_aabb_on_grid(box, rat::AabbEdge::MaxX, 2, 1.0f);
  REQUIRE(grown.min_x == Approx(0.0f));
  REQUIRE(grown.max_x == Approx(3.0f));
  REQUIRE(grown.min_z == Approx(0.0f));
  REQUIRE(grown.max_z == Approx(1.0f));
}

TEST_CASE("resize_aabb_on_grid keeps a minimum size of one tile", "[unit][blocker_edit]") {
  const rat::Aabb2 box{0.0f, 0.0f, 1.0f, 1.0f};
  const auto shrunk = rat::resize_aabb_on_grid(box, rat::AabbEdge::MaxX, -5, 1.0f);
  REQUIRE(shrunk.min_x == Approx(0.0f));
  REQUIRE(shrunk.max_x == Approx(1.0f));
}

TEST_CASE("snap_aabb_to_grid snaps all corners", "[unit][blocker_edit]") {
  const auto snapped = rat::snap_aabb_to_grid({0.2f, -0.7f, 1.8f, 1.1f}, 1.0f);
  REQUIRE(snapped.min_x == Approx(0.0f));
  REQUIRE(snapped.min_z == Approx(-1.0f));
  REQUIRE(snapped.max_x == Approx(2.0f));
  REQUIRE(snapped.max_z == Approx(1.0f));
}

TEST_CASE("set_blocker_vertical_range enforces valid pair", "[unit][blocker_edit]") {
  rat::BlockerDef blocker;
  blocker.bounds = {0.0f, 0.0f, 1.0f, 1.0f};

  REQUIRE_FALSE(rat::set_blocker_vertical_range(blocker, true, 0.0f, std::nullopt));
  REQUIRE_FALSE(blocker.jumpable);
  REQUIRE_FALSE(blocker.base_y.has_value());
  REQUIRE_FALSE(blocker.top_y.has_value());

  REQUIRE_FALSE(rat::set_blocker_vertical_range(blocker, true, 2.0f, 1.0f));
  REQUIRE_FALSE(blocker.jumpable);

  REQUIRE(rat::set_blocker_vertical_range(blocker, true, 0.25f, 1.25f));
  REQUIRE(blocker.jumpable);
  REQUIRE(blocker.base_y.has_value());
  REQUIRE(blocker.top_y.has_value());
  REQUIRE(*blocker.base_y == Approx(0.25f));
  REQUIRE(*blocker.top_y == Approx(1.25f));

  REQUIRE(rat::set_blocker_vertical_range(blocker, false, std::nullopt, std::nullopt));
  REQUIRE_FALSE(blocker.jumpable);
  REQUIRE_FALSE(blocker.base_y.has_value());
  REQUIRE_FALSE(blocker.top_y.has_value());
}
