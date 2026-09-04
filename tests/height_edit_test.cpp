#include <rat/event_runtime.hpp>
#include <rat/height_edit.hpp>
#include <rat/map_data.hpp>
#include <rat/map_document.hpp>

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

TEST_CASE("compile RuntimeMap after place_map_tile_cube raises flat tile by 1.0",
          "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  const auto result = rat::place_map_tile_cube(map, -1, 4);
  REQUIRE(result.ok);
  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);
  rat::EventRuntime runtime;
  runtime.load(compiled.runtime);
  REQUIRE(rat::get_tile_ground_y(runtime.map().height_grid, -1, 4).value == Approx(5.0f));
  REQUIRE(runtime.map().ramps.empty());
}

TEST_CASE("Upsert mini edge barrier last-wins on tile and direction", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::EdgeBarrierDef edge;
  edge.tile = {-2, 3};
  edge.direction = rat::RampDirection::East;
  edge.height = rat::kEdgeBarrierMiniHeight;

  const auto first = rat::upsert_map_edge_barrier(map, edge);
  REQUIRE(first.ok);
  REQUIRE(map.edge_barriers.size() == 1);
  REQUIRE(map.edge_barriers[0].tile.x == -2);
  REQUIRE(map.edge_barriers[0].tile.z == 3);
  REQUIRE(map.edge_barriers[0].direction == rat::RampDirection::East);
  REQUIRE(map.edge_barriers[0].height == Approx(rat::kEdgeBarrierMiniHeight));

  edge.height = rat::kEdgeBarrierFullHeight;
  const auto replaced = rat::upsert_map_edge_barrier(map, edge);
  REQUIRE(replaced.ok);
  REQUIRE(map.edge_barriers.size() == 1);
  REQUIRE(map.edge_barriers[0].height == Approx(rat::kEdgeBarrierFullHeight));

  rat::EdgeBarrierDef other = edge;
  other.direction = rat::RampDirection::North;
  other.height = rat::kEdgeBarrierMiniHeight;
  REQUIRE(rat::upsert_map_edge_barrier(map, other).ok);
  REQUIRE(map.edge_barriers.size() == 2);
}

TEST_CASE("Upsert edge barrier rejects ramp tile without mutation", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::RampDef ramp;
  ramp.tile = rat::TileCoord{-2, 3};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 2.0f;
  ramp.high_y = 4.0f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);

  const auto before_edges = map.edge_barriers;
  const auto before_ramps = map.ramps;
  rat::EdgeBarrierDef edge;
  edge.tile = {-2, 3};
  edge.direction = rat::RampDirection::North;
  edge.height = rat::kEdgeBarrierMiniHeight;
  const auto result = rat::upsert_map_edge_barrier(map, edge);
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.error.empty());
  REQUIRE(map.edge_barriers.size() == before_edges.size());
  REQUIRE(map.ramps.size() == before_ramps.size());
  REQUIRE(map.ramps[0].tile.x == before_ramps[0].tile.x);
  REQUIRE(map.ramps[0].tile.z == before_ramps[0].tile.z);
}

TEST_CASE("Remove edge barrier drops matching tile and direction", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::EdgeBarrierDef east;
  east.tile = {-2, 3};
  east.direction = rat::RampDirection::East;
  east.height = rat::kEdgeBarrierMiniHeight;
  rat::EdgeBarrierDef north = east;
  north.direction = rat::RampDirection::North;
  north.height = rat::kEdgeBarrierFullHeight;
  REQUIRE(rat::upsert_map_edge_barrier(map, east).ok);
  REQUIRE(rat::upsert_map_edge_barrier(map, north).ok);
  REQUIRE(map.edge_barriers.size() == 2);

  const auto removed = rat::remove_map_edge_barrier(map, east.tile, rat::RampDirection::East);
  REQUIRE(removed.ok);
  REQUIRE(map.edge_barriers.size() == 1);
  REQUIRE(map.edge_barriers[0].direction == rat::RampDirection::North);
  REQUIRE(map.edge_barriers[0].height == Approx(rat::kEdgeBarrierFullHeight));

  const auto missing = rat::remove_map_edge_barrier(map, east.tile, rat::RampDirection::East);
  REQUIRE_FALSE(missing.ok);
  REQUIRE_FALSE(missing.error.empty());
  REQUIRE(map.edge_barriers.size() == 1);
}

TEST_CASE("Upsert ramp drops edge barriers on that tile", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::EdgeBarrierDef keep;
  keep.tile = {-1, 4};
  keep.direction = rat::RampDirection::West;
  keep.height = rat::kEdgeBarrierMiniHeight;
  rat::EdgeBarrierDef drop_a;
  drop_a.tile = {-2, 3};
  drop_a.direction = rat::RampDirection::East;
  drop_a.height = rat::kEdgeBarrierMiniHeight;
  rat::EdgeBarrierDef drop_b = drop_a;
  drop_b.direction = rat::RampDirection::South;
  drop_b.height = rat::kEdgeBarrierFullHeight;
  REQUIRE(rat::upsert_map_edge_barrier(map, keep).ok);
  REQUIRE(rat::upsert_map_edge_barrier(map, drop_a).ok);
  REQUIRE(rat::upsert_map_edge_barrier(map, drop_b).ok);
  REQUIRE(map.edge_barriers.size() == 3);

  rat::RampDef ramp;
  ramp.tile = rat::TileCoord{-2, 3};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 1.0f;
  ramp.high_y = 2.0f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);
  REQUIRE(map.edge_barriers.size() == 1);
  REQUIRE(map.edge_barriers[0].tile.x == -1);
  REQUIRE(map.edge_barriers[0].tile.z == 4);
  REQUIRE(map.edge_barriers[0].direction == rat::RampDirection::West);
}

TEST_CASE("compile RuntimeMap after upsert and remove edge barrier", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::EdgeBarrierDef edge;
  edge.tile = {-1, 4};
  edge.direction = rat::RampDirection::South;
  edge.height = rat::kEdgeBarrierMiniHeight;
  const auto upserted = rat::upsert_map_edge_barrier(map, edge);
  REQUIRE(upserted.ok);
  const rat::MapCompileResult compiled_up = rat::compile_map_data(map);
  REQUIRE(compiled_up.ok);
  rat::EventRuntime runtime;
  runtime.load(compiled_up.runtime);
  REQUIRE(runtime.map().edge_barriers.size() == 1);
  REQUIRE(runtime.map().edge_barriers[0].height == Approx(rat::kEdgeBarrierMiniHeight));

  REQUIRE(rat::remove_map_edge_barrier(map, edge.tile, edge.direction).ok);
  const rat::MapCompileResult compiled_down = rat::compile_map_data(map);
  REQUIRE(compiled_down.ok);
  runtime.load(compiled_down.runtime);
  REQUIRE(runtime.map().edge_barriers.empty());
}

TEST_CASE("compile RuntimeMap after upsert_map_ramp drops fences on that tile",
          "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  rat::EdgeBarrierDef edge;
  edge.tile = {-2, 3};
  edge.direction = rat::RampDirection::North;
  edge.height = rat::kEdgeBarrierFullHeight;
  REQUIRE(rat::upsert_map_edge_barrier(map, edge).ok);
  REQUIRE(map.edge_barriers.size() == 1);

  rat::RampDef ramp;
  ramp.tile = edge.tile;
  ramp.direction = rat::RampDirection::West;
  ramp.low_y = 0.5f;
  ramp.high_y = 1.5f;
  REQUIRE(rat::upsert_map_ramp(map, ramp).ok);
  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);
  rat::EventRuntime runtime;
  runtime.load(compiled.runtime);
  REQUIRE(runtime.map().edge_barriers.empty());
}

TEST_CASE("Upsert floor slab toggles same top_y and bumps schema to 3", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  REQUIRE(map.schema_version == 2);
  REQUIRE(rat::set_tile_ground_y(map.height_grid, -2, 3, 0.0f).ok);
  rat::FloorSlabDef slab;
  slab.tile = {-2, 3};
  slab.top_y = 2.0f;
  slab.thickness = rat::kDefaultFloorSlabThickness;

  const auto first = rat::upsert_map_floor_slab(map, slab);
  REQUIRE(first.ok);
  REQUIRE(map.schema_version == 3);
  REQUIRE(map.floor_slabs.size() == 1);
  REQUIRE(map.floor_slabs[0].tile.x == -2);
  REQUIRE(map.floor_slabs[0].tile.z == 3);
  REQUIRE(map.floor_slabs[0].top_y == Approx(2.0f));
  REQUIRE(map.floor_slabs[0].thickness == Approx(rat::kDefaultFloorSlabThickness));

  const auto toggled = rat::upsert_map_floor_slab(map, slab);
  REQUIRE(toggled.ok);
  REQUIRE(map.floor_slabs.empty());
}

TEST_CASE("Upsert floor slab adds a second slab at a different top_y", "[unit][height_edit]") {
  rat::MapData map = make_v2_map();
  REQUIRE(rat::set_tile_ground_y(map.height_grid, -2, 3, 0.0f).ok);
  rat::FloorSlabDef lower;
  lower.tile = {-2, 3};
  lower.top_y = 1.6f;
  lower.thickness = 0.25f;
  rat::FloorSlabDef upper = lower;
  upper.top_y = 2.0f;
  REQUIRE(rat::upsert_map_floor_slab(map, lower).ok);
  REQUIRE(rat::upsert_map_floor_slab(map, upper).ok);
  REQUIRE(map.floor_slabs.size() == 2);
  REQUIRE(map.floor_slabs[0].top_y == Approx(1.6f));
  REQUIRE(map.floor_slabs[1].top_y == Approx(2.0f));
}

TEST_CASE("Upsert floor slab on open ground succeeds; cube cell is refused",
          "[unit][height_edit][terrain]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "bridge";
  map.width = 2;
  map.height = 1;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 2;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f, rat::kPlaceCubeDeltaY};

  rat::FloorSlabDef open_span;
  open_span.tile = {0, 0};
  open_span.top_y = 2.0f;
  open_span.thickness = 0.25f;
  const auto placed = rat::upsert_map_floor_slab(map, open_span);
  REQUIRE(placed.ok);
  REQUIRE(map.floor_slabs.size() == 1);

  rat::FloorSlabDef on_cube = open_span;
  on_cube.tile = {1, 0};
  const auto refused = rat::upsert_map_floor_slab(map, on_cube);
  REQUIRE_FALSE(refused.ok);
  REQUIRE(map.floor_slabs.size() == 1);
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
