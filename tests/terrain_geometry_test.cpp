#include <rat/height_edit.hpp>
#include <rat/map_loader.hpp>
#include <rat/terrain_geometry.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace {

bool same_grid_point(const rat::TerrainLineVertex& v, float x, float y, float z) {
  return v.x == Catch::Approx(x) && v.y == Catch::Approx(y) && v.z == Catch::Approx(z);
}

bool has_grid_segment(const std::vector<rat::TerrainLineSegment>& lines, float x0, float y0,
                      float z0, float x1, float y1, float z1) {
  for (const auto& seg : lines) {
    if ((same_grid_point(seg.a, x0, y0, z0) && same_grid_point(seg.b, x1, y1, z1)) ||
        (same_grid_point(seg.a, x1, y1, z1) && same_grid_point(seg.b, x0, y0, z0))) {
      return true;
    }
  }
  return false;
}

}  // namespace

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
  // 2 tiles × 4 own-corner edges. Shared lattice sampling hid the peak.
  REQUIRE(lines.size() == 8);

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

  const rat::TerrainSideFace* found = nullptr;
  for (const rat::TerrainSideFace& face : faces) {
    if (face.x0 == Catch::Approx(1.0f) && face.x1 == Catch::Approx(1.0f) &&
        face.z0 == Catch::Approx(0.0f) && face.z1 == Catch::Approx(1.0f)) {
      found = &face;
      break;
    }
  }
  REQUIRE(found != nullptr);
  const rat::TerrainSideFace& face = *found;
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

  int internal_south_count = 0;
  const rat::TerrainSideFace* found = nullptr;
  for (const rat::TerrainSideFace& face : faces) {
    if (face.z0 == Catch::Approx(1.0f) && face.z1 == Catch::Approx(1.0f) &&
        face.x0 == Catch::Approx(0.0f) && face.x1 == Catch::Approx(1.0f)) {
      ++internal_south_count;
      found = &face;
    }
  }
  REQUIRE(internal_south_count == 1);
  REQUIRE(found != nullptr);
  const rat::TerrainSideFace& face = *found;
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

  const rat::TerrainSideFace* found = nullptr;
  for (const rat::TerrainSideFace& face : faces) {
    if (face.x0 == Catch::Approx(1.0f) && face.x1 == Catch::Approx(1.0f) &&
        face.z0 == Catch::Approx(0.0f) && face.z1 == Catch::Approx(1.0f)) {
      found = &face;
      break;
    }
  }
  REQUIRE(found != nullptr);
  const rat::TerrainSideFace& face = *found;
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

  const rat::TerrainSideFace* found = nullptr;
  for (const rat::TerrainSideFace& face : faces) {
    if (face.x0 == Catch::Approx(22.0f) && face.x1 == Catch::Approx(22.0f) &&
        face.z0 == Catch::Approx(-4.0f) && face.z1 == Catch::Approx(-2.0f)) {
      found = &face;
      break;
    }
  }
  REQUIRE(found != nullptr);
  const rat::TerrainSideFace& face = *found;
  REQUIRE(face.y0_hi - face.y0_lo == Catch::Approx(1.0f));
  REQUIRE(face.y1_hi - face.y1_lo == Catch::Approx(1.0f));
}

TEST_CASE("Edge barrier faces emit east fence from owner top to mini height", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {0.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const std::vector<rat::EdgeBarrierDef> barriers = {
      {.tile = {0, 0},
       .direction = rat::RampDirection::East,
       .height = rat::kEdgeBarrierMiniHeight},
  };
  const auto faces = rat::build_edge_barrier_faces(geometry, barriers);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.x0 == Catch::Approx(1.0f));
  REQUIRE(face.x1 == Catch::Approx(1.0f));
  REQUIRE(face.z0 == Catch::Approx(0.0f));
  REQUIRE(face.z1 == Catch::Approx(1.0f));
  REQUIRE(face.y0_lo == Catch::Approx(0.0f));
  REQUIRE(face.y0_hi == Catch::Approx(rat::kEdgeBarrierMiniHeight));
  REQUIRE(face.y1_lo == Catch::Approx(0.0f));
  REQUIRE(face.y1_hi == Catch::Approx(rat::kEdgeBarrierMiniHeight));
}

TEST_CASE("Edge barrier faces emit north fence from raised owner top", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {1.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const std::vector<rat::EdgeBarrierDef> barriers = {
      {.tile = {0, 0},
       .direction = rat::RampDirection::North,
       .height = rat::kEdgeBarrierFullHeight},
  };
  const auto faces = rat::build_edge_barrier_faces(geometry, barriers);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.z0 == Catch::Approx(0.0f));
  REQUIRE(face.z1 == Catch::Approx(0.0f));
  REQUIRE(face.x0 == Catch::Approx(0.0f));
  REQUIRE(face.x1 == Catch::Approx(1.0f));
  REQUIRE(face.y0_lo == Catch::Approx(1.0f));
  REQUIRE(face.y0_hi == Catch::Approx(1.0f + rat::kEdgeBarrierFullHeight));
  REQUIRE(face.y1_lo == Catch::Approx(1.0f));
  REQUIRE(face.y1_hi == Catch::Approx(1.0f + rat::kEdgeBarrierFullHeight));
}

TEST_CASE("Edge barrier faces honor origin, tile_size, west and south", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 10;
  grid.origin_z = -2;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {0.5f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 2.0f);
  const std::vector<rat::EdgeBarrierDef> barriers = {
      {.tile = {10, -2},
       .direction = rat::RampDirection::West,
       .height = rat::kEdgeBarrierMiniHeight},
      {.tile = {10, -2},
       .direction = rat::RampDirection::South,
       .height = rat::kEdgeBarrierFullHeight},
  };
  const auto faces = rat::build_edge_barrier_faces(geometry, barriers);
  REQUIRE(faces.size() == 2);

  const rat::TerrainSideFace& west = faces[0];
  REQUIRE(west.x0 == Catch::Approx(20.0f));
  REQUIRE(west.x1 == Catch::Approx(20.0f));
  REQUIRE(west.z0 == Catch::Approx(-4.0f));
  REQUIRE(west.z1 == Catch::Approx(-2.0f));
  REQUIRE(west.y0_lo == Catch::Approx(0.5f));
  REQUIRE(west.y0_hi == Catch::Approx(0.5f + rat::kEdgeBarrierMiniHeight));

  const rat::TerrainSideFace& south = faces[1];
  REQUIRE(south.z0 == Catch::Approx(-2.0f));
  REQUIRE(south.z1 == Catch::Approx(-2.0f));
  REQUIRE(south.x0 == Catch::Approx(20.0f));
  REQUIRE(south.x1 == Catch::Approx(22.0f));
  REQUIRE(south.y0_hi - south.y0_lo == Catch::Approx(rat::kEdgeBarrierFullHeight));
  REQUIRE(south.y0_lo == Catch::Approx(0.5f));
}

TEST_CASE("Edge barrier faces skip non-positive height and missing tiles", "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {0.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const std::vector<rat::EdgeBarrierDef> barriers = {
      {.tile = {0, 0}, .direction = rat::RampDirection::East, .height = 0.0f},
      {.tile = {9, 9},
       .direction = rat::RampDirection::East,
       .height = rat::kEdgeBarrierMiniHeight},
  };
  const auto faces = rat::build_edge_barrier_faces(geometry, barriers);
  REQUIRE(faces.empty());
}

TEST_CASE("Terrain fill mesh rejects extra fence faces that overflow uint16", "[unit][terrain]") {
  REQUIRE(rat::terrain_fill_quad_count_fits_u16(16382, 1, 0));
  REQUIRE_FALSE(rat::terrain_fill_quad_count_fits_u16(16382, 1, 1));
  REQUIRE(rat::terrain_fill_quad_count_fits_u16(16381, 1, 1));
  REQUIRE_FALSE(rat::terrain_fill_quad_count_fits_u16(16383, 0, 1));
}

TEST_CASE("Edge barrier faces sit on raised tile edge corners, not a center sample",
          "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {1.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  REQUIRE(geometry.tiles.size() == 1);
  const rat::TerrainTileQuad& tile = geometry.tiles[0];

  const std::vector<rat::EdgeBarrierDef> barriers = {
      {.tile = {0, 0},
       .direction = rat::RampDirection::East,
       .height = rat::kEdgeBarrierMiniHeight},
  };
  const auto faces = rat::build_edge_barrier_faces(geometry, barriers);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.y0_lo == Catch::Approx(tile.y_ne));
  REQUIRE(face.y1_lo == Catch::Approx(tile.y_se));
  REQUIRE(face.y0_hi == Catch::Approx(tile.y_ne + rat::kEdgeBarrierMiniHeight));
  REQUIRE(face.y1_hi == Catch::Approx(tile.y_se + rat::kEdgeBarrierMiniHeight));
}

TEST_CASE("Edge barrier faces use both owner-edge corners when those Y differ",
          "[unit][terrain]") {
  rat::TerrainGeometry geometry;
  geometry.origin_x = 0;
  geometry.origin_z = 0;
  geometry.width = 1;
  geometry.height = 1;
  geometry.tile_size = 1.0f;
  rat::TerrainTileQuad tile;
  tile.min_x = 0.0f;
  tile.max_x = 1.0f;
  tile.min_z = 0.0f;
  tile.max_z = 1.0f;
  tile.y_nw = 4.0f;
  tile.y_ne = 1.0f;
  tile.y_se = 2.0f;
  tile.y_sw = 3.0f;
  geometry.tiles.push_back(tile);

  const std::vector<rat::EdgeBarrierDef> barriers = {
      {.tile = {0, 0},
       .direction = rat::RampDirection::East,
       .height = rat::kEdgeBarrierMiniHeight},
  };
  const auto faces = rat::build_edge_barrier_faces(geometry, barriers);
  REQUIRE(faces.size() == 1);

  const rat::TerrainSideFace& face = faces[0];
  REQUIRE(face.y0_lo == Catch::Approx(1.0f));
  REQUIRE(face.y1_lo == Catch::Approx(2.0f));
  REQUIRE(face.y0_hi == Catch::Approx(1.0f + rat::kEdgeBarrierMiniHeight));
  REQUIRE(face.y1_hi == Catch::Approx(2.0f + rat::kEdgeBarrierMiniHeight));
}

TEST_CASE("Terrain side faces emit an outer east wall down to implied height 0",
          "[unit][terrain]") {
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 1;
  grid.height = 1;
  grid.ground_y = {1.0f};

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, {}, 1.0f);
  const auto faces = rat::build_terrain_side_faces(geometry);

  bool found_east = false;
  for (const rat::TerrainSideFace& face : faces) {
    if (face.x0 == Catch::Approx(1.0f) && face.x1 == Catch::Approx(1.0f) &&
        face.z0 == Catch::Approx(0.0f) && face.z1 == Catch::Approx(1.0f)) {
      REQUIRE(face.y0_lo == Catch::Approx(0.0f));
      REQUIRE(face.y0_hi == Catch::Approx(1.0f));
      REQUIRE(face.y1_lo == Catch::Approx(0.0f));
      REQUIRE(face.y1_hi == Catch::Approx(1.0f));
      found_east = true;
    }
  }
  REQUIRE(found_east);
}

TEST_CASE("Terrain grid lines keep a raised cell's top edges at the high Y", "[unit][terrain]") {
  // Row z=0: east ramp 0→1 then a flat cube at y=1. Row z=1: floor at y=0.
  // Global lattice sampling at z=1 / x=1 floor()s into the low neighbor and
  // interpolates a diagonal that hides the high side.
  rat::HeightGrid grid;
  grid.origin_x = 0;
  grid.origin_z = 0;
  grid.width = 2;
  grid.height = 2;
  grid.ground_y = {0.0f, 1.0f, 0.0f, 0.0f};
  const std::vector<rat::RampDef> ramps = {
      rat::RampDef{.tile = rat::TileCoord{0, 0},
                   .direction = rat::RampDirection::East,
                   .low_y = 0.0f,
                   .high_y = 1.0f},
  };

  const rat::TerrainGeometry geometry = rat::build_terrain_geometry(grid, ramps, 1.0f);
  REQUIRE(geometry.tiles.size() == 4);
  const rat::TerrainTileQuad& ramp = geometry.tiles[0];
  const rat::TerrainTileQuad& raised = geometry.tiles[1];
  REQUIRE(ramp.y_nw == Catch::Approx(0.0f));
  REQUIRE(ramp.y_ne == Catch::Approx(1.0f));
  REQUIRE(ramp.y_se == Catch::Approx(1.0f));
  REQUIRE(ramp.y_sw == Catch::Approx(0.0f));
  REQUIRE(raised.y_nw == Catch::Approx(1.0f));
  REQUIRE(raised.y_ne == Catch::Approx(1.0f));
  REQUIRE(raised.y_se == Catch::Approx(1.0f));
  REQUIRE(raised.y_sw == Catch::Approx(1.0f));

  const auto lines = rat::build_terrain_grid_lines(geometry);
  constexpr float kOff = 0.03f;

  REQUIRE(has_grid_segment(lines, 1.0f, 1.0f + kOff, 0.0f, 1.0f, 1.0f + kOff, 1.0f));
  REQUIRE(has_grid_segment(lines, 1.0f, 1.0f + kOff, 1.0f, 2.0f, 1.0f + kOff, 1.0f));
}

TEST_CASE("grey_yard ramp fill stays in uint16 and high cells keep a y=1 grid",
          "[unit][terrain]") {
  const rat::MapLoadResult loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);
  const rat::MapData& map = loaded.map;
  REQUIRE(rat::choose_terrain_render_policy(map) == rat::TerrainRenderPolicy::HeightTerrain);

  const rat::TerrainGeometry geometry =
      rat::build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  REQUIRE_FALSE(geometry.tiles.empty());

  const auto side_faces = rat::build_terrain_side_faces(geometry);
  const auto fence_faces = rat::build_edge_barrier_faces(geometry, map.edge_barriers);
  constexpr std::size_t kQuadsPerBox = 6;
  const std::size_t extra_quads =
      fence_faces.size() + map.floor_slabs.size() * kQuadsPerBox +
      map.ladders.size() * kQuadsPerBox;
  REQUIRE(rat::terrain_fill_quad_count_fits_u16(geometry.tiles.size(), side_faces.size(),
                                                extra_quads));

  const std::size_t ramp_index = static_cast<std::size_t>(8 - geometry.origin_z) *
                                     static_cast<std::size_t>(geometry.width) +
                                 static_cast<std::size_t>(8 - geometry.origin_x);
  REQUIRE(ramp_index < geometry.tiles.size());
  const rat::TerrainTileQuad& ramp = geometry.tiles[ramp_index];
  REQUIRE(ramp.on_ramp);
  REQUIRE(ramp.min_x == Catch::Approx(8.0f));
  REQUIRE(ramp.min_z == Catch::Approx(8.0f));
  REQUIRE(ramp.y_nw == Catch::Approx(0.0f));
  REQUIRE(ramp.y_sw == Catch::Approx(0.0f));
  REQUIRE(ramp.y_ne == Catch::Approx(1.0f));
  REQUIRE(ramp.y_se == Catch::Approx(1.0f));

  const auto lines = rat::build_terrain_grid_lines(geometry);
  constexpr float kOff = 0.03f;
  REQUIRE(has_grid_segment(lines, 9.0f, 1.0f + kOff, 8.0f, 9.0f, 1.0f + kOff, 9.0f));
  REQUIRE(has_grid_segment(lines, 9.0f, 1.0f + kOff, 9.0f, 10.0f, 1.0f + kOff, 9.0f));
  REQUIRE(has_grid_segment(lines, 10.0f, 1.0f + kOff, 9.0f, 11.0f, 1.0f + kOff, 9.0f));
  REQUIRE(has_grid_segment(lines, 11.0f, 1.0f + kOff, 9.0f, 12.0f, 1.0f + kOff, 9.0f));
}

TEST_CASE("Floor slab over open ground contributes top and bottom fill quads",
          "[unit][terrain]") {
  const rat::FloorSlabDef slab{{0, 0}, 2.0f, 0.25f};
  const std::vector<rat::TerrainFillQuad> quads = rat::build_floor_slab_fill_quads(slab, 1.0f);
  REQUIRE(quads.size() == rat::kFloorSlabFillQuadCount);

  bool has_top = false;
  bool has_bottom = false;
  for (const rat::TerrainFillQuad& quad : quads) {
    const bool top = quad.y0 == Catch::Approx(2.0f) && quad.y1 == Catch::Approx(2.0f) &&
                     quad.y2 == Catch::Approx(2.0f) && quad.y3 == Catch::Approx(2.0f);
    const bool bottom = quad.y0 == Catch::Approx(1.75f) && quad.y1 == Catch::Approx(1.75f) &&
                        quad.y2 == Catch::Approx(1.75f) && quad.y3 == Catch::Approx(1.75f);
    if (top) {
      has_top = true;
    }
    if (bottom) {
      has_bottom = true;
    }
  }
  REQUIRE(has_top);
  REQUIRE(has_bottom);
}

TEST_CASE("Occupancy solid fill quads are a 1 m cube at voxel Y", "[unit][terrain][edit]") {
  const rat::OccupancyCell cell{.x = 2, .y = 1, .z = 3, .kind = rat::OccupancyKind::Solid};
  const std::vector<rat::TerrainFillQuad> quads =
      rat::build_occupancy_solid_fill_quads(cell, 1.0f);
  REQUIRE(quads.size() == rat::kOccupancySolidFillQuadCount);

  bool has_top = false;
  bool has_bottom = false;
  for (const rat::TerrainFillQuad& quad : quads) {
    const bool top = quad.y0 == Catch::Approx(2.0f) && quad.y1 == Catch::Approx(2.0f) &&
                     quad.y2 == Catch::Approx(2.0f) && quad.y3 == Catch::Approx(2.0f);
    const bool bottom = quad.y0 == Catch::Approx(1.0f) && quad.y1 == Catch::Approx(1.0f) &&
                        quad.y2 == Catch::Approx(1.0f) && quad.y3 == Catch::Approx(1.0f);
    if (top) {
      has_top = true;
      REQUIRE(quad.x0 == Catch::Approx(2.0f));
      REQUIRE(quad.x2 == Catch::Approx(3.0f));
      REQUIRE(quad.z0 == Catch::Approx(3.0f));
      REQUIRE(quad.z2 == Catch::Approx(4.0f));
    }
    if (bottom) {
      has_bottom = true;
    }
  }
  REQUIRE(has_top);
  REQUIRE(has_bottom);
}

TEST_CASE("Occupancy ramp fill quads are a wedge spanning one metre of Y, not a cube",
          "[unit][terrain][edit]") {
  const rat::OccupancyCell cell{.x = 2,
                                .y = 1,
                                .z = 3,
                                .kind = rat::OccupancyKind::Ramp,
                                .yaw = rat::RampDirection::East};
  const std::vector<rat::TerrainFillQuad> quads =
      rat::build_occupancy_ramp_fill_quads(cell, 1.0f);
  REQUIRE(quads.size() == rat::kOccupancyRampFillQuadCount);
  REQUIRE(rat::build_occupancy_solid_fill_quads(cell, 1.0f).empty());

  bool has_sloped_top = false;
  bool has_flat_cube_top = false;
  for (const rat::TerrainFillQuad& quad : quads) {
    const bool all_y2 = quad.y0 == Catch::Approx(2.0f) && quad.y1 == Catch::Approx(2.0f) &&
                        quad.y2 == Catch::Approx(2.0f) && quad.y3 == Catch::Approx(2.0f);
    if (all_y2) {
      has_flat_cube_top = true;
    }
    const bool low_west = (quad.y0 == Catch::Approx(1.0f) && quad.x0 == Catch::Approx(2.0f)) ||
                          (quad.y1 == Catch::Approx(1.0f) && quad.x1 == Catch::Approx(2.0f)) ||
                          (quad.y2 == Catch::Approx(1.0f) && quad.x2 == Catch::Approx(2.0f)) ||
                          (quad.y3 == Catch::Approx(1.0f) && quad.x3 == Catch::Approx(2.0f));
    const bool high_east = (quad.y0 == Catch::Approx(2.0f) && quad.x0 == Catch::Approx(3.0f)) ||
                           (quad.y1 == Catch::Approx(2.0f) && quad.x1 == Catch::Approx(3.0f)) ||
                           (quad.y2 == Catch::Approx(2.0f) && quad.x2 == Catch::Approx(3.0f)) ||
                           (quad.y3 == Catch::Approx(2.0f) && quad.x3 == Catch::Approx(3.0f));
    if (low_west && high_east) {
      has_sloped_top = true;
    }
  }
  REQUIRE(has_sloped_top);
  REQUIRE_FALSE(has_flat_cube_top);
}

