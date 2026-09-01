#include <rat/map_loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("smoke: grey_yard.json loads without a window", "[smoke]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto result =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(result.ok);
  REQUIRE(result.map.id == "grey_yard");
  REQUIRE_FALSE(result.map.height_grid.ground_y.empty());
  REQUIRE_FALSE(result.map.events.empty());
}
