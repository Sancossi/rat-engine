#include <rat/event_runtime.hpp>
#include <rat/height_edit.hpp>
#include <rat/map_data.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace {

rat::MapData make_v2_map() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "height_edit";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = -2;
  map.height_grid.origin_z = 3;
  map.height_grid.width = 2;
  map.height_grid.height = 2;
  map.height_grid.ground_y = {
      1.0f, 2.0f,
      3.0f, 4.0f,
  };
  return map;
}

}  // namespace

TEST_CASE("Height edit get/set supports negative origin", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();

  const auto before = rat::get_tile_ground_y(map.height_grid, -1, 4);
  REQUIRE(before.ok);
  REQUIRE(before.value == Approx(4.0f));

  const auto set_result = rat::set_tile_ground_y(map.height_grid, -2, 3, 7.5f);
  REQUIRE(set_result.ok);

  const auto after = rat::get_tile_ground_y(map.height_grid, -2, 3);
  REQUIRE(after.ok);
  REQUIRE(after.value == Approx(7.5f));
}

TEST_CASE("Height edit adjust applies exact step delta", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  const auto adjusted = rat::adjust_tile_ground_y(map.height_grid, -2, 4, 0.25f);
  REQUIRE(adjusted.ok);
  REQUIRE(rat::get_tile_ground_y(map.height_grid, -2, 4).value == Approx(3.25f));

  const auto adjusted_back = rat::adjust_tile_ground_y(map.height_grid, -2, 4, -0.5f);
  REQUIRE(adjusted_back.ok);
  REQUIRE(rat::get_tile_ground_y(map.height_grid, -2, 4).value == Approx(2.75f));
}

TEST_CASE("Height edit out-of-range returns error and keeps data", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  const auto original = map.height_grid.ground_y;

  const auto set_result = rat::set_tile_ground_y(map.height_grid, 99, 99, 12.0f);
  REQUIRE_FALSE(set_result.ok);
  REQUIRE_FALSE(set_result.error.empty());
  REQUIRE(map.height_grid.ground_y == original);

  const auto get_result = rat::get_tile_ground_y(map.height_grid, 99, 99);
  REQUIRE_FALSE(get_result.ok);
  REQUIRE_FALSE(get_result.error.empty());
}

TEST_CASE("Height edit ramp add replace remove is deterministic", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  map.ramps.clear();

  const rat::TileCoord tile{-1, 4};
  const rat::RampDirection directions[] = {
      rat::RampDirection::North,
      rat::RampDirection::East,
      rat::RampDirection::South,
      rat::RampDirection::West,
  };

  for (rat::RampDirection direction : directions) {
    rat::RampDef ramp;
    ramp.tile = tile;
    ramp.direction = direction;
    ramp.low_y = 0.0f;
    ramp.high_y = 1.0f;
    const auto upsert = rat::add_or_replace_ramp(map.ramps, map.height_grid, ramp);
    REQUIRE(upsert.ok);
    REQUIRE(map.ramps.size() == 1);
    REQUIRE(map.ramps[0].tile.x == tile.x);
    REQUIRE(map.ramps[0].tile.z == tile.z);
    REQUIRE(map.ramps[0].direction == direction);
  }

  REQUIRE(rat::remove_ramp_by_tile(map.ramps, tile));
  REQUIRE(map.ramps.empty());
  REQUIRE_FALSE(rat::remove_ramp_by_tile(map.ramps, tile));
}

TEST_CASE("Height edit rejects ramp with high below low", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::RampDef ramp;
  ramp.tile = rat::TileCoord{-2, 3};
  ramp.direction = rat::RampDirection::North;
  ramp.low_y = 2.0f;
  ramp.high_y = 1.0f;

  const auto result = rat::add_or_replace_ramp(map.ramps, map.height_grid, ramp);
  REQUIRE_FALSE(result.ok);
  REQUIRE(map.ramps.empty());
}

TEST_CASE("Height edit map set/adjust reject ramp tile without mutation", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::RampDef ramp;
  ramp.tile = rat::TileCoord{-2, 3};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 2.0f;
  ramp.high_y = 4.0f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);

  const auto before_grid = map.height_grid.ground_y;
  const auto set_result = rat::set_map_tile_ground_y(map, -2, 3, 9.0f);
  REQUIRE_FALSE(set_result.ok);
  REQUIRE_FALSE(set_result.error.empty());
  REQUIRE(map.height_grid.ground_y == before_grid);

  const auto adjust_result = rat::adjust_map_tile_ground_y(map, -2, 3, 1.0f);
  REQUIRE_FALSE(adjust_result.ok);
  REQUIRE_FALSE(adjust_result.error.empty());
  REQUIRE(map.height_grid.ground_y == before_grid);
}

TEST_CASE("Upsert ramp aligns base ground and remove keeps low flat", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  const rat::TileCoord tile{-2, 3};
  REQUIRE(rat::set_map_tile_ground_y(map, tile.x, tile.z, 7.0f).ok);

  rat::RampDef ramp;
  ramp.tile = tile;
  ramp.direction = rat::RampDirection::South;
  ramp.low_y = 1.25f;
  ramp.high_y = 2.75f;
  const auto upsert = rat::upsert_map_ramp(map, ramp);
  REQUIRE(upsert.ok);
  REQUIRE(rat::get_tile_ground_y(map.height_grid, tile.x, tile.z).value == Approx(1.25f));

  const auto removed = rat::remove_map_ramp(map, tile);
  REQUIRE(removed.ok);
  REQUIRE(map.ramps.empty());
  REQUIRE(rat::get_tile_ground_y(map.height_grid, tile.x, tile.z).value == Approx(1.25f));
}

TEST_CASE("Height edit rejects invalid ramp enum direction", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::RampDef ramp;
  ramp.tile = rat::TileCoord{-2, 3};
  ramp.direction = static_cast<rat::RampDirection>(777);
  ramp.low_y = 0.0f;
  ramp.high_y = 1.0f;

  const auto result = rat::add_or_replace_ramp(map.ramps, map.height_grid, ramp);
  REQUIRE_FALSE(result.ok);
  REQUIRE(map.ramps.empty());
}

TEST_CASE("Height edit replace removes duplicate tile ramps", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  map.ramps = {
      {.tile = {-1, 4}, .direction = rat::RampDirection::North, .low_y = 0.0f, .high_y = 1.0f},
      {.tile = {-1, 4}, .direction = rat::RampDirection::South, .low_y = 3.0f, .high_y = 4.0f},
      {.tile = {-2, 3}, .direction = rat::RampDirection::East, .low_y = 1.0f, .high_y = 2.0f},
  };

  rat::RampDef replacement;
  replacement.tile = {-1, 4};
  replacement.direction = rat::RampDirection::West;
  replacement.low_y = 5.0f;
  replacement.high_y = 7.0f;

  const auto result = rat::add_or_replace_ramp(map.ramps, map.height_grid, replacement);
  REQUIRE(result.ok);
  REQUIRE(map.ramps.size() == 2);
  REQUIRE(map.ramps[0].tile.x == -1);
  REQUIRE(map.ramps[0].tile.z == 4);
  REQUIRE(map.ramps[0].direction == rat::RampDirection::West);
  REQUIRE(map.ramps[0].low_y == Approx(5.0f));
  REQUIRE(map.ramps[0].high_y == Approx(7.0f));
}

TEST_CASE("Place cube raises selected flat tile by exactly 1.0", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  const auto before = rat::get_tile_ground_y(map.height_grid, -2, 3);
  REQUIRE(before.ok);
  REQUIRE(before.value == Approx(1.0f));
  REQUIRE(map.ramps.empty());

  const auto placed = rat::place_map_tile_cube(map, -2, 3);
  REQUIRE(placed.ok);
  REQUIRE(rat::get_tile_ground_y(map.height_grid, -2, 3).value == Approx(2.0f));
  REQUIRE(map.ramps.empty());

  const auto stacked = rat::place_map_tile_cube(map, -2, 3);
  REQUIRE(stacked.ok);
  REQUIRE(rat::get_tile_ground_y(map.height_grid, -2, 3).value == Approx(3.0f));
  REQUIRE(map.ramps.empty());
}

TEST_CASE("Place cube rejects ramp tile without mutation", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::RampDef ramp;
  ramp.tile = rat::TileCoord{-2, 3};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 2.0f;
  ramp.high_y = 4.0f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);

  const auto before_grid = map.height_grid.ground_y;
  const auto before_ramps = map.ramps;
  const auto placed = rat::place_map_tile_cube(map, -2, 3);
  REQUIRE_FALSE(placed.ok);
  REQUIRE_FALSE(placed.error.empty());
  REQUIRE(map.height_grid.ground_y == before_grid);
  REQUIRE(map.ramps.size() == before_ramps.size());
  REQUIRE(map.ramps[0].tile.x == before_ramps[0].tile.x);
  REQUIRE(map.ramps[0].tile.z == before_ramps[0].tile.z);
  REQUIRE(map.ramps[0].direction == before_ramps[0].direction);
  REQUIRE(map.ramps[0].low_y == Approx(before_ramps[0].low_y));
  REQUIRE(map.ramps[0].high_y == Approx(before_ramps[0].high_y));
}

TEST_CASE("EventRuntime place_tile_cube raises flat tile by 1.0", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::EventRuntime runtime;
  runtime.load(map);

  const auto result = runtime.place_tile_cube(-1, 4);
  REQUIRE(result.ok);
  REQUIRE(rat::get_tile_ground_y(runtime.map().height_grid, -1, 4).value == Approx(5.0f));
  REQUIRE(runtime.map().ramps.empty());
}

TEST_CASE("Height edit schema upgrade keeps existing values", "[unit][height_edit]") {
  rat::MapData map;
  map.schema_version = 1;
  map.id = "legacy";
  map.width = 3;
  map.height = 2;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 3;
  map.height_grid.height = 2;
  map.height_grid.ground_y = {
      0.0f, 1.0f, 2.0f,
      3.0f, 4.0f, 5.0f,
  };

  const auto upgraded = rat::upgrade_map_schema_for_elevation(map);
  REQUIRE(upgraded.ok);
  REQUIRE(map.schema_version == 2);
  REQUIRE(map.height_grid.width == 3);
  REQUIRE(map.height_grid.height == 2);
  REQUIRE(map.height_grid.ground_y.size() == 6);
  REQUIRE(map.height_grid.ground_y[0] == Approx(0.0f));
  REQUIRE(map.height_grid.ground_y[5] == Approx(5.0f));
}
