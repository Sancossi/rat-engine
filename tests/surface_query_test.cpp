#include <rat/map_data.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <utility>

TEST_CASE("SurfaceQuery returns defaults outside grid", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "empty";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {2.0f};

  const rat::SurfaceQuery query(map);
  const rat::SurfaceSample sample = query.sample(5.5f, -3.0f);
  REQUIRE(sample.y == Catch::Approx(0.0f));
  REQUIRE(sample.surface_id == 0);
  REQUIRE(sample.walkable);
}

TEST_CASE("SurfaceQuery samples flat tile height", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "flat";
  map.width = 2;
  map.height = 2;
  map.height_grid.origin_x = -1;
  map.height_grid.origin_z = 4;
  map.height_grid.width = 2;
  map.height_grid.height = 2;
  map.height_grid.ground_y = {1.0f, 2.0f, 3.0f, 4.0f};

  const rat::SurfaceQuery query(map);
  const rat::SurfaceSample sample = query.sample(0.2f, 4.8f);
  REQUIRE(sample.y == Catch::Approx(2.0f));
  REQUIRE(sample.surface_id == 0);
  REQUIRE(sample.walkable);
}

TEST_CASE("SurfaceQuery uses tile-space origin with tile_size", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "scaled";
  map.width = 2;
  map.height = 2;
  map.tile_size = 2.0f;
  map.height_grid.origin_x = 10;
  map.height_grid.origin_z = -4;
  map.height_grid.width = 2;
  map.height_grid.height = 2;
  map.height_grid.ground_y = {
      1.0f, 2.0f,
      3.0f, 4.0f,
  };

  const rat::SurfaceQuery query(map);
  const rat::SurfaceSample sample = query.sample(22.2f, -5.8f);
  REQUIRE(sample.y == Catch::Approx(4.0f));
  REQUIRE(sample.surface_id == 0);
  REQUIRE(sample.walkable);
}

TEST_CASE("SurfaceQuery interpolates ramp endpoints and midpoint", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "ramp";
  map.width = 3;
  map.height = 3;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 3;
  map.height_grid.height = 3;
  map.height_grid.ground_y = {
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
  };
  map.ramps.push_back({
      .tile = rat::TileCoord{1, 1},
      .direction = rat::RampDirection::East,
      .low_y = 10.0f,
      .high_y = 14.0f,
  });

  const rat::SurfaceQuery query(map);

  const rat::SurfaceSample low = query.sample(1.0f, 1.5f);
  REQUIRE(low.y == Catch::Approx(10.0f));
  REQUIRE(low.walkable);
  REQUIRE(low.on_ramp);

  const rat::SurfaceSample mid = query.sample(1.5f, 1.5f);
  REQUIRE(mid.y == Catch::Approx(12.0f));
  REQUIRE(mid.walkable);
  REQUIRE(mid.on_ramp);

  const rat::SurfaceSample high = query.sample(1.99f, 1.5f);
  REQUIRE(high.y == Catch::Approx(13.96f).margin(0.02f));
  REQUIRE(high.walkable);
  REQUIRE(high.on_ramp);
}

TEST_CASE("SurfaceQuery marks non-ramp samples", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "flat_only";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {3.0f};

  const rat::SurfaceQuery query(map);
  const rat::SurfaceSample sample = query.sample(0.2f, 0.2f);
  REQUIRE(sample.y == Catch::Approx(3.0f));
  REQUIRE_FALSE(sample.on_ramp);
}

TEST_CASE("SurfaceQuery interpolates north ramp", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "north";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::North,
      .low_y = 2.0f,
      .high_y = 6.0f,
  });

  const rat::SurfaceQuery query(map);
  REQUIRE(query.sample(0.5f, 0.95f).y == Catch::Approx(2.2f).margin(0.05f));
  REQUIRE(query.sample(0.5f, 0.5f).y == Catch::Approx(4.0f));
  REQUIRE(query.sample(0.5f, 0.05f).y == Catch::Approx(5.8f).margin(0.05f));
}

TEST_CASE("SurfaceQuery interpolates south ramp", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "south";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::South,
      .low_y = 2.0f,
      .high_y = 6.0f,
  });

  const rat::SurfaceQuery query(map);
  REQUIRE(query.sample(0.5f, 0.05f).y == Catch::Approx(2.2f).margin(0.05f));
  REQUIRE(query.sample(0.5f, 0.5f).y == Catch::Approx(4.0f));
  REQUIRE(query.sample(0.5f, 0.95f).y == Catch::Approx(5.8f).margin(0.05f));
}

TEST_CASE("SurfaceQuery interpolates west ramp", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "west";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::West,
      .low_y = 2.0f,
      .high_y = 6.0f,
  });

  const rat::SurfaceQuery query(map);
  REQUIRE(query.sample(0.95f, 0.5f).y == Catch::Approx(2.2f).margin(0.05f));
  REQUIRE(query.sample(0.5f, 0.5f).y == Catch::Approx(4.0f));
  REQUIRE(query.sample(0.05f, 0.5f).y == Catch::Approx(5.8f).margin(0.05f));
}

TEST_CASE("SurfaceQuery moved-from object is safe to sample", "[unit][surface]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "move_safety";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {9.0f};

  rat::SurfaceQuery source(map);
  rat::SurfaceQuery moved(std::move(source));
  const rat::SurfaceSample moved_sample = moved.sample(0.25f, 0.25f);
  REQUIRE(moved_sample.y == Catch::Approx(9.0f));
  REQUIRE(moved_sample.walkable);

  const rat::SurfaceSample source_after_move = source.sample(0.25f, 0.25f);
  REQUIRE(source_after_move.y == Catch::Approx(0.0f));
  REQUIRE(source_after_move.surface_id == 0);
  REQUIRE(source_after_move.walkable);
}
