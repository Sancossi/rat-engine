#include <rat/terrain_geometry.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("TerrainGeometry builds flat quads from HeightGrid", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 2;
  grid.height = 1;
  grid.ground_y = {1.0f, 2.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  REQUIRE(geometry.tiles.size() == 2);

  const rat::TerrainTileQuad& tile0 = geometry.tiles[0];
  REQUIRE(tile0.min_x == Catch::Approx(0.0f));
  REQUIRE(tile0.max_x == Catch::Approx(1.0f));
  REQUIRE(tile0.min_z == Catch::Approx(0.0f));
  REQUIRE(tile0.max_z == Catch::Approx(1.0f));
  REQUIRE(tile0.y_nw == Catch::Approx(1.0f));
  REQUIRE(tile0.y_ne == Catch::Approx(1.0f));
  REQUIRE(tile0.y_se == Catch::Approx(1.0f));
  REQUIRE(tile0.y_sw == Catch::Approx(1.0f));
}

TEST_CASE("TerrainGeometry applies tile origin and tile_size", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 10;
  grid.origin_z = -2;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {3.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 2.0f);
  REQUIRE(geometry.tiles.size() == 1);
  const rat::TerrainTileQuad& tile = geometry.tiles[0];
  REQUIRE(tile.min_x == Catch::Approx(20.0f));
  REQUIRE(tile.max_x == Catch::Approx(22.0f));
  REQUIRE(tile.min_z == Catch::Approx(-4.0f));
  REQUIRE(tile.max_z == Catch::Approx(-2.0f));
}

TEST_CASE("TerrainGeometry maps east ramp to quad corners", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {0.0f};
  const std::vector<rat::RampDef> ramps = {
      rat::RampDef{.tile = rat::TileCoord{0, 0},
                   .direction = rat::RampDirection::East,
                   .low_y = 2.0f,
                   .high_y = 6.0f},
  };

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, ramps, 1.0f);
  REQUIRE(geometry.tiles.size() == 1);
  const rat::TerrainTileQuad& tile = geometry.tiles[0];
  REQUIRE(tile.on_ramp);
  REQUIRE(tile.y_nw == Catch::Approx(2.0f));
  REQUIRE(tile.y_sw == Catch::Approx(2.0f));
  REQUIRE(tile.y_ne == Catch::Approx(6.0f));
  REQUIRE(tile.y_se == Catch::Approx(6.0f));
}

TEST_CASE("TerrainGeometry maps all ramp directions to matching edges", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {0.0f};

  auto build_single = [&](rat::RampDirection dir) {
    const std::vector<rat::RampDef> ramps = {
        rat::RampDef{.tile = rat::TileCoord{0, 0}, .direction = dir, .low_y = 1.0f, .high_y = 5.0f},
    };
    return rat::build_terrain_geometry(grid, ramps, 1.0f).tiles[0];
  };

  const rat::TerrainTileQuad north = build_single(rat::RampDirection::North);
  REQUIRE(north.y_nw == Catch::Approx(5.0f));
  REQUIRE(north.y_ne == Catch::Approx(5.0f));
  REQUIRE(north.y_sw == Catch::Approx(1.0f));
  REQUIRE(north.y_se == Catch::Approx(1.0f));

  const rat::TerrainTileQuad south = build_single(rat::RampDirection::South);
  REQUIRE(south.y_nw == Catch::Approx(1.0f));
  REQUIRE(south.y_ne == Catch::Approx(1.0f));
  REQUIRE(south.y_sw == Catch::Approx(5.0f));
  REQUIRE(south.y_se == Catch::Approx(5.0f));

  const rat::TerrainTileQuad west = build_single(rat::RampDirection::West);
  REQUIRE(west.y_nw == Catch::Approx(5.0f));
  REQUIRE(west.y_sw == Catch::Approx(5.0f));
  REQUIRE(west.y_ne == Catch::Approx(1.0f));
  REQUIRE(west.y_se == Catch::Approx(1.0f));
}

TEST_CASE("TerrainGeometry sampling follows ramp interpolation", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {0.0f};
  const std::vector<rat::RampDef> ramps = {
      rat::RampDef{.tile = rat::TileCoord{0, 0},
                   .direction = rat::RampDirection::North,
                   .low_y = 10.0f,
                   .high_y = 14.0f},
  };

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, ramps, 1.0f);
  REQUIRE(rat::sample_terrain_height(geometry, 0.5f, 0.95f) ==
          Catch::Approx(10.2f).margin(0.05f));
  REQUIRE(rat::sample_terrain_height(geometry, 0.5f, 0.5f) == Catch::Approx(12.0f));
  REQUIRE(rat::sample_terrain_height(geometry, 0.5f, 0.05f) ==
          Catch::Approx(13.8f).margin(0.05f));
}

TEST_CASE("Terrain policy keeps v1 on legacy floor/grid", "[unit][terrain]") {
  rat::MapData map;
  map.schema_version = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 2;
  map.height_grid.height = 2;
  map.height_grid.ground_y = {0.0f, 0.0f, 0.0f, 0.0f};

  REQUIRE(rat::choose_terrain_render_policy(map) == rat::TerrainRenderPolicy::LegacyFlat);
}

TEST_CASE("Terrain policy enables v2 height terrain", "[unit][terrain]") {
  rat::MapData map;
  map.schema_version = 2;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};

  REQUIRE(rat::choose_terrain_render_policy(map) == rat::TerrainRenderPolicy::HeightTerrain);
}

TEST_CASE("Terrain grid lines keep ramp profile per tile", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 2;
  grid.height = 1;
  grid.ground_y = {0.0f, 0.0f};
  const std::vector<rat::RampDef> ramps = {
      rat::RampDef{.tile = rat::TileCoord{0, 0},
                   .direction = rat::RampDirection::East,
                   .low_y = 0.0f,
                   .high_y = 2.0f},
      rat::RampDef{.tile = rat::TileCoord{1, 0},
                   .direction = rat::RampDirection::West,
                   .low_y = 0.0f,
                   .high_y = 2.0f},
  };
  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, ramps, 1.0f);

  const auto lines = rat::build_terrain_grid_lines(geometry);
  // 2x1 grid => (h+1)*w + (w+1)*h = 4 + 3 = 7 segments.
  REQUIRE(lines.size() == 7);

  // Vertex at x=1 (tile seam) must retain peak profile instead of straight endpoint line.
  bool found_seam_peak = false;
  for (const auto& seg : lines) {
    if (seg.a.x == Catch::Approx(1.0f) && seg.a.z == Catch::Approx(0.0f) &&
        seg.a.y > 1.9f) {
      found_seam_peak = true;
      break;
    }
    if (seg.b.x == Catch::Approx(1.0f) && seg.b.z == Catch::Approx(0.0f) &&
        seg.b.y > 1.9f) {
      found_seam_peak = true;
      break;
    }
  }
  REQUIRE(found_seam_peak);
}

TEST_CASE("Terrain clamped sampling stays inside last cell", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 5;
  grid.origin_z = -2;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {3.0f};
  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 2.0f);

  REQUIRE(rat::sample_terrain_height_clamped(geometry, 12.0f, -2.0f) == Catch::Approx(3.0f));
}

TEST_CASE("Terrain tile-count guard protects uint16 index range", "[unit][terrain]") {
  REQUIRE(rat::terrain_tile_count_fits_u16(16383));
  REQUIRE_FALSE(rat::terrain_tile_count_fits_u16(16384));
}

TEST_CASE("Terrain fill mesh rejects extra side faces that overflow uint16", "[unit][terrain]") {
  REQUIRE(rat::terrain_fill_quad_count_fits_u16(16383, 0));
  REQUIRE_FALSE(rat::terrain_fill_quad_count_fits_u16(16383, 1));
  REQUIRE(rat::terrain_fill_quad_count_fits_u16(16382, 1));
  REQUIRE_FALSE(rat::terrain_fill_quad_count_fits_u16(16384, 0));
}

TEST_CASE("Terrain side faces skip equal-height shared edges", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 2;
  grid.height = 2;
  grid.ground_y = {0.0f, 0.0f, 0.0f, 0.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);
  REQUIRE(faces.empty());
}

TEST_CASE("Terrain side faces emit one wall for a flat 0 vs 1 neighbor", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 2;
  grid.height = 1;
  grid.ground_y = {0.0f, 1.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.x0 == Catch::Approx(1.0f));
  REQUIRE(face.x1 == Catch::Approx(1.0f));
  REQUIRE(face.z0 == Catch::Approx(0.0f));
  REQUIRE(face.z1 == Catch::Approx(1.0f));
  REQUIRE(face.y0_lo == Catch::Approx(0.0f));
  REQUIRE(face.y0_hi == Catch::Approx(1.0f));
  REQUIRE(face.y1_lo == Catch::Approx(0.0f));
  REQUIRE(face.y1_hi == Catch::Approx(1.0f));
}

TEST_CASE("Terrain side faces pair only east and south so walls are not duplicated",
          "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 2;
  grid.ground_y = {0.0f, 1.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.z0 == Catch::Approx(1.0f));
  REQUIRE(face.z1 == Catch::Approx(1.0f));
  REQUIRE(face.x0 == Catch::Approx(0.0f));
  REQUIRE(face.x1 == Catch::Approx(1.0f));
  REQUIRE(face.y0_lo == Catch::Approx(0.0f));
  REQUIRE(face.y0_hi == Catch::Approx(1.0f));
  REQUIRE(face.y1_lo == Catch::Approx(0.0f));
  REQUIRE(face.y1_hi == Catch::Approx(1.0f));
}

TEST_CASE("Terrain side faces surround an interior raised cube", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 3;
  grid.height = 3;
  grid.ground_y = {
      0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
  };

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);
  REQUIRE(faces.size() == 4);
}

TEST_CASE("Terrain side faces use shared-edge ramp corners as a trapezoid", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 2;
  grid.height = 1;
  grid.ground_y = {0.0f, 0.0f};
  const std::vector<rat::RampDef> ramps = {
      rat::RampDef{.tile = rat::TileCoord{0, 0},
                   .direction = rat::RampDirection::North,
                   .low_y = 0.0f,
                   .high_y = 2.0f},
  };

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, ramps, 1.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.x0 == Catch::Approx(1.0f));
  REQUIRE(face.x1 == Catch::Approx(1.0f));
  REQUIRE(face.z0 == Catch::Approx(0.0f));
  REQUIRE(face.z1 == Catch::Approx(1.0f));
  REQUIRE(face.y0_lo == Catch::Approx(0.0f));
  REQUIRE(face.y0_hi == Catch::Approx(2.0f));
  REQUIRE(face.y1_lo == Catch::Approx(0.0f));
  REQUIRE(face.y1_hi == Catch::Approx(0.0f));
}

TEST_CASE("Terrain side faces honor origin and tile_size", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 10;
  grid.origin_z = -2;
  grid.width = 2;
  grid.height = 1;
  grid.ground_y = {0.0f, 1.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 2.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.x0 == Catch::Approx(22.0f));
  REQUIRE(face.x1 == Catch::Approx(22.0f));
  REQUIRE(face.z0 == Catch::Approx(-4.0f));
  REQUIRE(face.z1 == Catch::Approx(-2.0f));
  REQUIRE(face.y0_hi - face.y0_lo == Catch::Approx(1.0f));
  REQUIRE(face.y1_hi - face.y1_lo == Catch::Approx(1.0f));
}
