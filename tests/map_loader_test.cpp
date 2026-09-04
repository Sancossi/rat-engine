#include <rat/blocker_edit.hpp>
#include <rat/collision.hpp>
#include <rat/height_edit.hpp>
#include <rat/indoor_volume.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(in.good());
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

bool utf8_contains_cyrillic(std::string_view text) {
  for (std::size_t i = 0; i < text.size();) {
    const unsigned char lead = static_cast<unsigned char>(text[i]);
    unsigned codepoint = 0;
    std::size_t width = 1;
    if (lead < 0x80u) {
      codepoint = lead;
    } else if ((lead & 0xE0u) == 0xC0u && i + 1 < text.size()) {
      width = 2;
      codepoint = (static_cast<unsigned>(lead & 0x1Fu) << 6) |
                  (static_cast<unsigned>(static_cast<unsigned char>(text[i + 1])) & 0x3Fu);
    } else if ((lead & 0xF0u) == 0xE0u && i + 2 < text.size()) {
      width = 3;
      codepoint = (static_cast<unsigned>(lead & 0x0Fu) << 12) |
                  ((static_cast<unsigned>(static_cast<unsigned char>(text[i + 1])) & 0x3Fu) << 6) |
                  (static_cast<unsigned>(static_cast<unsigned char>(text[i + 2])) & 0x3Fu);
    } else if ((lead & 0xF8u) == 0xF0u && i + 3 < text.size()) {
      i += 4;
      continue;
    } else {
      ++i;
      continue;
    }
    i += width;
    if (codepoint >= 0x0400u && codepoint <= 0x04FFu) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_CASE("Map loader parses minimal map JSON", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "mini",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 1, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [
              { "op": "show_text", "text": "Hi" },
              { "op": "control_switch", "id": 9, "value": true }
            ]
          }
        ]
      }
    ]
  })";

  const auto result = rat::load_map_from_string(kJson);
  REQUIRE(result.ok);
  REQUIRE(result.map.id == "mini");
  REQUIRE(result.map.width == 4);
  REQUIRE(result.map.events.size() == 1);
  REQUIRE(result.map.events[0].id == "npc");
  REQUIRE(result.map.events[0].tile.has_value());
  REQUIRE(result.map.events[0].tile->x == 1);
  REQUIRE(result.map.events[0].pages.size() == 1);
  REQUIRE(result.map.events[0].pages[0].trigger == rat::TriggerKind::Action);
  REQUIRE(result.map.events[0].pages[0].commands.size() == 2);
  REQUIRE(result.map.events[0].pages[0].commands[0].op == rat::CommandOp::ShowText);
  REQUIRE(result.map.events[0].pages[0].commands[0].text == "Hi");
  REQUIRE(result.map.events[0].pages[0].commands[1].op == rat::CommandOp::ControlSwitch);
  REQUIRE(result.map.events[0].pages[0].commands[1].id == 9);
  REQUIRE(result.map.events[0].pages[0].commands[1].bool_value == true);
  REQUIRE_FALSE(result.map.events[0].y.has_value());
}

TEST_CASE("Event optional y round-trips through serialize", "[unit][map][event]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "loft_y",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0, 0, 0, 0]
    },
    "events": [
      {
        "id": "on_loft",
        "tile": { "x": 0, "z": 0 },
        "y": 2.0,
        "pages": [{ "trigger": "action", "commands": [] }]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events[0].y.has_value());
  REQUIRE(*loaded.map.events[0].y == Catch::Approx(2.0f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events[0].y.has_value());
  REQUIRE(*again.map.events[0].y == Catch::Approx(2.0f));
}

TEST_CASE("Map loader rejects unknown schema version", "[unit][map]") {
  const auto result = rat::load_map_from_string(R"({"schema_version":6,"id":"x","width":1,"height":1})");
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("Map loader rejects v2 map without height grid", "[unit][map]") {
  const auto result =
      rat::load_map_from_string(R"({"schema_version":2,"id":"x","width":1,"height":1,"events":[]})");
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("Map loader v1 fallback builds flat height grid", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "legacy",
    "width": 3,
    "height": 2,
    "events": []
  })";

  const auto result = rat::load_map_from_string(kJson);
  REQUIRE(result.ok);
  REQUIRE(result.map.schema_version == 1);
  REQUIRE(result.map.height_grid.origin_x == 0);
  REQUIRE(result.map.height_grid.origin_z == 0);
  REQUIRE(result.map.height_grid.width == 3);
  REQUIRE(result.map.height_grid.height == 2);
  REQUIRE(result.map.height_grid.ground_y.size() == 6);
  for (const float y : result.map.height_grid.ground_y) {
    REQUIRE(y == Catch::Approx(0.0f));
  }
}

TEST_CASE("Example grey_yard.json loads without crash", "[unit][map]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined for map file tests
#endif
  const std::string path = std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json";
  const auto result = rat::load_map_from_file(path);
  REQUIRE(result.ok);
  REQUIRE(result.map.id == "grey_yard");
  REQUIRE(result.map.schema_version == 4);
  REQUIRE(result.map.height_grid.origin_x == -4);
  REQUIRE(result.map.height_grid.origin_z == -3);
  REQUIRE(result.map.height_grid.width == 16);
  REQUIRE(result.map.height_grid.height == 16);
  REQUIRE(result.map.height_grid.ground_y.size() == static_cast<std::size_t>(16 * 16));
  REQUIRE_FALSE(result.map.blockers.empty());
  REQUIRE(result.map.events.size() >= 3);

  bool found_autorun = false;
  bool found_branchish = false;
  bool found_crate_notice = false;
  bool found_jumpable_blocker = false;
  bool found_elevated_event = false;
  bool found_loft_event = false;
  int max_event_x = std::numeric_limits<int>::min();
  int max_event_z = std::numeric_limits<int>::min();
  int min_event_x = std::numeric_limits<int>::max();
  int min_event_z = std::numeric_limits<int>::max();
  for (const auto& ev : result.map.events) {
    if (ev.tile.has_value()) {
      max_event_x = std::max(max_event_x, ev.tile->x);
      max_event_z = std::max(max_event_z, ev.tile->z);
      min_event_x = std::min(min_event_x, ev.tile->x);
      min_event_z = std::min(min_event_z, ev.tile->z);
    }
    if (ev.id == "elevated_after_blocker") {
      REQUIRE(ev.tile.has_value());
      REQUIRE(ev.tile->x == 11);
      REQUIRE(ev.tile->z == 8);
      REQUIRE(ev.pages.size() == 1);
      REQUIRE(ev.pages[0].graph.has_value());
      REQUIRE(ev.pages[0].graph->nodes.size() == 2);
      REQUIRE(ev.pages[0].graph->nodes[1].kind == "control_switch");
      REQUIRE(ev.pages[0].graph->nodes[1].switch_id == 40);
      REQUIRE(ev.pages[0].graph->nodes[1].bool_value);
      found_elevated_event = true;
    }
    if (ev.id == "loft_plank") {
      REQUIRE(ev.tile.has_value());
      REQUIRE(ev.tile->x == 0);
      REQUIRE(ev.tile->z == 4);
      REQUIRE(ev.y.has_value());
      REQUIRE(*ev.y == Catch::Approx(2.0f));
      REQUIRE(ev.pages.size() == 1);
      REQUIRE(ev.pages[0].trigger == rat::TriggerKind::Action);
      REQUIRE(ev.pages[0].graph.has_value());
      REQUIRE(ev.pages[0].graph->nodes.size() == 2);
      REQUIRE(ev.pages[0].graph->nodes[1].kind == "control_switch");
      REQUIRE(ev.pages[0].graph->nodes[1].switch_id == 43);
      found_loft_event = true;
    }
    if (ev.id == "crate_notice") {
      REQUIRE(ev.volume.has_value());
      REQUIRE(ev.volume->min_x == Catch::Approx(3.0f));
      REQUIRE(ev.volume->min_z == Catch::Approx(-1.0f));
      REQUIRE(ev.volume->max_x == Catch::Approx(5.0f));
      REQUIRE(ev.volume->max_z == Catch::Approx(1.0f));
      found_crate_notice = true;
    }
    for (const auto& page : ev.pages) {
      if (page.trigger == rat::TriggerKind::Autorun) {
        found_autorun = true;
      }
      if (!page.conditions.empty()) {
        found_branchish = true;
      }
    }
  }
  for (const auto& blocker : result.map.blockers) {
    if (blocker.jumpable && blocker.base_y.has_value() && blocker.top_y.has_value()) {
      REQUIRE(blocker.bounds.min_x == Catch::Approx(10.2f));
      REQUIRE(blocker.bounds.min_z == Catch::Approx(8.1f));
      REQUIRE(blocker.bounds.max_x == Catch::Approx(10.8f));
      REQUIRE(blocker.bounds.max_z == Catch::Approx(8.9f));
      REQUIRE(*blocker.base_y == Catch::Approx(1.0f));
      REQUIRE(*blocker.top_y == Catch::Approx(1.6f));
      found_jumpable_blocker = true;
    }
  }
  REQUIRE(result.map.ramps.size() >= 2);
  REQUIRE(result.map.ramps[0].tile.x == 8);
  REQUIRE(result.map.ramps[0].tile.z == 8);
  REQUIRE(result.map.ramps[0].direction == rat::RampDirection::East);
  REQUIRE(result.map.ramps[0].low_y == Catch::Approx(0.0f));
  REQUIRE(result.map.ramps[0].high_y == Catch::Approx(1.0f));
  {
    const int local_x = result.map.ramps[0].tile.x - result.map.height_grid.origin_x;
    const int local_z = result.map.ramps[0].tile.z - result.map.height_grid.origin_z;
    const std::size_t index = static_cast<std::size_t>(local_z) *
                                  static_cast<std::size_t>(result.map.height_grid.width) +
                              static_cast<std::size_t>(local_x);
    REQUIRE(index < result.map.height_grid.ground_y.size());
    REQUIRE(result.map.height_grid.ground_y[index] ==
            Catch::Approx(result.map.ramps[0].low_y));
  }
  const int grid_max_x = result.map.height_grid.origin_x + result.map.height_grid.width - 1;
  const int grid_max_z = result.map.height_grid.origin_z + result.map.height_grid.height - 1;
  REQUIRE(min_event_x >= result.map.height_grid.origin_x);
  REQUIRE(min_event_z >= result.map.height_grid.origin_z);
  REQUIRE(max_event_x <= grid_max_x);
  REQUIRE(max_event_z <= grid_max_z);
  REQUIRE(max_event_x == 11);
  REQUIRE(max_event_z == 8);

  bool found_standable_cube = false;
  {
    const rat::HeightGrid& grid = result.map.height_grid;
    const int cube_x = 9;
    const int cube_z = 8;
    const int local_x = cube_x - grid.origin_x;
    const int local_z = cube_z - grid.origin_z;
    const int neighbor_x = cube_x - grid.origin_x;
    const int neighbor_z = (cube_z - 1) - grid.origin_z;
    REQUIRE(local_x >= 0);
    REQUIRE(local_z >= 0);
    REQUIRE(local_x < grid.width);
    REQUIRE(local_z < grid.height);
    REQUIRE(neighbor_z >= 0);
    const std::size_t cube_index =
        static_cast<std::size_t>(local_z) * static_cast<std::size_t>(grid.width) +
        static_cast<std::size_t>(local_x);
    const std::size_t neighbor_index =
        static_cast<std::size_t>(neighbor_z) * static_cast<std::size_t>(grid.width) +
        static_cast<std::size_t>(neighbor_x);
    REQUIRE(grid.ground_y[cube_index] == Catch::Approx(1.0f));
    REQUIRE(grid.ground_y[neighbor_index] == Catch::Approx(0.0f));
    found_standable_cube = true;
  }

  bool found_mini_fence = false;
  bool found_full_fence = false;
  for (const rat::EdgeBarrierDef& edge : result.map.edge_barriers) {
    REQUIRE_FALSE((edge.tile.x == 8 && edge.tile.z == 8));
    REQUIRE_FALSE((edge.tile.x == 0 && edge.tile.z == 0));
    if (edge.height == Catch::Approx(rat::kEdgeBarrierMiniHeight)) {
      found_mini_fence = true;
    }
    if (edge.height == Catch::Approx(rat::kEdgeBarrierFullHeight)) {
      found_full_fence = true;
    }
  }

  REQUIRE(found_autorun);
  REQUIRE(found_branchish);
  REQUIRE(found_crate_notice);
  REQUIRE(found_jumpable_blocker);
  REQUIRE(found_elevated_event);
  REQUIRE(found_loft_event);
  REQUIRE(found_standable_cube);
  REQUIRE(found_mini_fence);
  REQUIRE(found_full_fence);
  REQUIRE(result.map.floor_slabs.size() >= 8);
  REQUIRE(result.map.floor_slabs[0].top_y == Catch::Approx(2.0f));
  REQUIRE(result.map.ladders.size() == 1);
  REQUIRE(result.map.ladders[0].tile.x == 2);
  REQUIRE(result.map.ladders[0].tile.z == 4);
  REQUIRE(result.map.ladders[0].direction == rat::RampDirection::East);

  // Round-trip via string path also works.
  const auto from_string = rat::load_map_from_string(read_file(path));
  REQUIRE(from_string.ok);
  REQUIRE(from_string.map.events.size() == result.map.events.size());
  REQUIRE(from_string.map.schema_version == 4);
  REQUIRE(from_string.map.height_grid.ground_y == result.map.height_grid.ground_y);

  const auto serialized = rat::serialize_map_to_string(result.map);
  REQUIRE(serialized.ok);
  const auto from_serialized = rat::load_map_from_string(serialized.json_text);
  REQUIRE(from_serialized.ok);
  REQUIRE(from_serialized.map.schema_version == 4);
  REQUIRE(from_serialized.map.floor_slabs.size() == result.map.floor_slabs.size());
  REQUIRE(from_serialized.map.ladders.size() == result.map.ladders.size());
  REQUIRE(from_serialized.map.height_grid.origin_x == result.map.height_grid.origin_x);
  REQUIRE(from_serialized.map.height_grid.origin_z == result.map.height_grid.origin_z);
  REQUIRE(from_serialized.map.height_grid.width == result.map.height_grid.width);
  REQUIRE(from_serialized.map.height_grid.height == result.map.height_grid.height);
  REQUIRE(from_serialized.map.height_grid.ground_y == result.map.height_grid.ground_y);
  REQUIRE(from_serialized.map.ramps.size() == result.map.ramps.size());
  REQUIRE(from_serialized.map.ramps[0].tile.x == 8);
  REQUIRE(from_serialized.map.ramps[0].tile.z == 8);
  REQUIRE(from_serialized.map.edge_barriers.size() == result.map.edge_barriers.size());
  REQUIRE(from_serialized.map.edge_barriers.size() >= 2);
  REQUIRE(from_serialized.map.edge_barriers[0].tile.x == result.map.edge_barriers[0].tile.x);
  REQUIRE(from_serialized.map.edge_barriers[0].tile.z == result.map.edge_barriers[0].tile.z);
  REQUIRE(from_serialized.map.edge_barriers[0].direction == result.map.edge_barriers[0].direction);
  REQUIRE(from_serialized.map.edge_barriers[0].height ==
          Catch::Approx(result.map.edge_barriers[0].height));
  REQUIRE(from_serialized.map.blockers.size() == result.map.blockers.size());
  REQUIRE(from_serialized.map.events.size() == result.map.events.size());
  REQUIRE(from_serialized.map.events.back().id == "loft_plank");
  REQUIRE(from_serialized.map.events.back().y.has_value());
  REQUIRE(*from_serialized.map.events.back().y == Catch::Approx(2.0f));
}

TEST_CASE("grey_yard show_text strings are Cyrillic", "[unit][map]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined for map file tests
#endif
  const std::string path = std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json";
  const auto result = rat::load_map_from_file(path);
  REQUIRE(result.ok);

  int show_text_count = 0;
  for (const auto& ev : result.map.events) {
    for (const auto& page : ev.pages) {
      REQUIRE(page.graph.has_value());
      for (const auto& node : page.graph->nodes) {
        if (node.kind != "show_text") {
          continue;
        }
        ++show_text_count;
        REQUIRE(utf8_contains_cyrillic(node.text));
      }
    }
  }
  REQUIRE(show_text_count >= 12);
}

TEST_CASE("grey_yard layout pass has house indoor, two gantry ramps, and a bridge",
          "[unit][map]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined for map file tests
#endif
  const std::string path = std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json";
  const auto result = rat::load_map_from_file(path);
  REQUIRE(result.ok);
  const rat::MapData& map = result.map;
  REQUIRE(map.schema_version == 4);
  REQUIRE(map.indoor_volumes.size() >= 1);

  const rat::IndoorVolume& house = map.indoor_volumes[0];
  REQUIRE(house.xz.max_x - house.xz.min_x >= 1.0f);
  REQUIRE(house.xz.max_z - house.xz.min_z >= 0.5f);
  REQUIRE(house.y_hi >= house.y_lo + rat::kPlayerCylinderHeight);
  // Inset so a facade quad center on the interior tile edge is outside the volume.
  REQUIRE(house.xz.min_x > std::floor(house.xz.min_x) + 0.1f);
  REQUIRE(house.xz.max_x < std::ceil(house.xz.max_x) - 0.1f);
  REQUIRE(house.xz.min_z > std::floor(house.xz.min_z) + 0.1f);
  REQUIRE(house.xz.max_z < std::ceil(house.xz.max_z) - 0.1f);

  const float indoor_x = 0.5f * (house.xz.min_x + house.xz.max_x);
  const float indoor_z = 0.5f * (house.xz.min_z + house.xz.max_z);
  const int indoor_tx = static_cast<int>(std::floor(indoor_x));
  const int indoor_tz = static_cast<int>(std::floor(indoor_z));
  const auto indoor_ground = rat::get_tile_ground_y(map.height_grid, indoor_tx, indoor_tz);
  REQUIRE(indoor_ground.ok);
  REQUIRE(indoor_ground.value == Catch::Approx(0.0f));

  rat::PlayerBody indoor_player;
  indoor_player.x = indoor_x;
  indoor_player.y = 0.0f;
  indoor_player.z = indoor_z;
  REQUIRE(rat::player_inside_indoor_volume(map, indoor_player));
  REQUIRE(rat::point_inside_indoor_volume(map, indoor_x, 0.5f, indoor_z));
  REQUIRE_FALSE(rat::point_inside_indoor_volume(map, std::floor(house.xz.min_x), 0.5f,
                                                 indoor_z));

  bool house_has_wall_cube = false;
  bool house_has_door = false;
  constexpr int kWallDelta[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  for (const int* delta : kWallDelta) {
    const int nx = indoor_tx + delta[0];
    const int nz = indoor_tz + delta[1];
    const auto neighbor = rat::get_tile_ground_y(map.height_grid, nx, nz);
    if (neighbor.ok && neighbor.value == Catch::Approx(rat::kPlaceCubeDeltaY)) {
      house_has_wall_cube = true;
    }
    if (neighbor.ok && neighbor.value == Catch::Approx(0.0f)) {
      const bool neighbor_inside = rat::point_inside_indoor_volume(
          map, static_cast<float>(nx) + 0.5f, 0.5f, static_cast<float>(nz) + 0.5f);
      if (!neighbor_inside) {
        house_has_door = true;
      }
    }
  }
  REQUIRE(house_has_wall_cube);
  REQUIRE(house_has_door);

  const rat::EventDef* spawn = nullptr;
  const rat::EventDef* scrap = nullptr;
  const rat::EventDef* loft = nullptr;
  for (const rat::EventDef& ev : map.events) {
    if (ev.id == "yard_intro") {
      spawn = &ev;
    } else if (ev.id == "scrap_pile") {
      scrap = &ev;
    } else if (ev.id == "loft_plank") {
      loft = &ev;
    }
  }
  REQUIRE(spawn != nullptr);
  REQUIRE(spawn->tile.has_value());
  REQUIRE(spawn->tile->x == 0);
  REQUIRE(spawn->tile->z == 0);
  REQUIRE(scrap != nullptr);
  REQUIRE(scrap->tile.has_value());
  REQUIRE(scrap->tile->x == 6);
  REQUIRE(scrap->tile->z == 0);
  REQUIRE(loft != nullptr);
  REQUIRE(loft->tile.has_value());
  REQUIRE(loft->tile->x == 0);
  REQUIRE(loft->tile->z == 4);

  int gantry_approach_ramps = 0;
  bool east_gantry_ramp = false;
  bool south_gantry_ramp = false;
  for (const rat::RampDef& ramp : map.ramps) {
    if (ramp.low_y != Catch::Approx(0.0f) || ramp.high_y != Catch::Approx(1.0f)) {
      continue;
    }
    int high_x = ramp.tile.x;
    int high_z = ramp.tile.z;
    switch (ramp.direction) {
      case rat::RampDirection::North:
        ++high_z;
        break;
      case rat::RampDirection::East:
        ++high_x;
        break;
      case rat::RampDirection::South:
        --high_z;
        break;
      case rat::RampDirection::West:
        --high_x;
        break;
    }
    const auto high = rat::get_tile_ground_y(map.height_grid, high_x, high_z);
    const auto low = rat::get_tile_ground_y(map.height_grid, ramp.tile.x, ramp.tile.z);
    if (!high.ok || !low.ok || high.value != Catch::Approx(1.0f) ||
        low.value != Catch::Approx(0.0f)) {
      continue;
    }
    ++gantry_approach_ramps;
    if (ramp.tile.x == 8 && ramp.tile.z == 8 && ramp.direction == rat::RampDirection::East) {
      east_gantry_ramp = true;
    }
    if (ramp.tile.x == 9 && ramp.tile.z == 7 && ramp.direction == rat::RampDirection::North) {
      south_gantry_ramp = true;
    }
  }
  REQUIRE(gantry_approach_ramps >= 2);
  REQUIRE(east_gantry_ramp);
  REQUIRE(south_gantry_ramp);
  REQUIRE(map.ramps.size() >= 2);

  bool found_bridge = false;
  bool loft_on_gantry = false;
  for (const rat::FloorSlabDef& slab : map.floor_slabs) {
    const auto ground = rat::get_tile_ground_y(map.height_grid, slab.tile.x, slab.tile.z);
    REQUIRE(ground.ok);
    const bool loft_cluster =
        slab.tile.x >= 0 && slab.tile.x <= 2 && slab.tile.z >= 4 && slab.tile.z <= 5;
    if (ground.value == Catch::Approx(1.0f) && loft_cluster) {
      loft_on_gantry = true;
    }
    if (loft_cluster) {
      continue;
    }
    const float slab_cx = static_cast<float>(slab.tile.x) + 0.5f;
    const float slab_cz = static_cast<float>(slab.tile.z) + 0.5f;
    const bool over_crates = slab_cx >= 3.0f && slab_cx <= 5.0f && slab_cz >= -1.0f &&
                             slab_cz <= 1.0f;
    if (!over_crates && ground.value <= rat::kBridgeOpenGroundMaxY &&
        slab.top_y - slab.thickness > rat::kPlayerCylinderHeight + 0.05f) {
      found_bridge = true;
    }
  }
  REQUIRE(found_bridge);
  REQUIRE_FALSE(loft_on_gantry);

  const auto cube_98 = rat::get_tile_ground_y(map.height_grid, 9, 8);
  const auto cube_108 = rat::get_tile_ground_y(map.height_grid, 10, 8);
  const auto cube_118 = rat::get_tile_ground_y(map.height_grid, 11, 8);
  REQUIRE(cube_98.ok);
  REQUIRE(cube_98.value == Catch::Approx(1.0f));
  REQUIRE(cube_108.ok);
  REQUIRE(cube_108.value == Catch::Approx(1.0f));
  REQUIRE(cube_118.ok);
  REQUIRE(cube_118.value == Catch::Approx(1.0f));
}

TEST_CASE("Serialize map roundtrips blockers after edit", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "mini",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "blockers": [{"min_x": 0, "min_z": 0, "max_x": 1, "max_z": 1}],
    "events": []
  })";
  auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  loaded.map.blockers[0].bounds =
      rat::translate_aabb_on_grid(loaded.map.blockers[0].bounds, 2, 0, 1.0f);

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);

  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.blockers.size() == 1);
  REQUIRE(again.map.blockers[0].bounds.min_x == Catch::Approx(2.0f));
  REQUIRE(again.map.blockers[0].bounds.max_x == Catch::Approx(3.0f));
}

TEST_CASE("Save map file roundtrips blocker and event edits", "[unit][map]") {
  rat::MapData map;
  map.id = "saved";
  map.width = 8;
  map.height = 8;
  map.blockers.push_back({.bounds = {2.0f, 3.0f, 4.0f, 5.0f}});

  rat::EventDef event;
  event.id = "moved";
  event.tile = rat::TileCoord{6, -2};
  event.pages.push_back({});
  map.events.push_back(event);

  const auto path = std::filesystem::temp_directory_path() / "rat-map-save-test.json";
  const auto saved = rat::save_map_to_file(map, path.string());
  REQUIRE(saved.ok);
  REQUIRE(saved.error.empty());

  const auto loaded = rat::load_map_from_file(path.string());
  std::filesystem::remove(path);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.blockers.size() == 1);
  REQUIRE(loaded.map.blockers[0].bounds.min_x == Catch::Approx(2.0f));
  REQUIRE(loaded.map.events.size() == 1);
  REQUIRE(loaded.map.events[0].tile->x == 6);
  REQUIRE(loaded.map.events[0].tile->z == -2);
}

TEST_CASE("Map loader v2 roundtrips height grid and ramps", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "height_map",
    "width": 8,
    "height": 8,
    "height_grid": {
      "origin_x": -2,
      "origin_z": 3,
      "width": 2,
      "height": 2,
      "ground_y": [1.0, 2.0, 3.0, 4.0]
    },
    "ramps": [
      {
        "tile": { "x": -1, "z": 4 },
        "direction": "east",
        "low_y": 2.0,
        "high_y": 3.5
      }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 2);
  REQUIRE(loaded.map.height_grid.origin_x == -2);
  REQUIRE(loaded.map.height_grid.origin_z == 3);
  REQUIRE(loaded.map.height_grid.width == 2);
  REQUIRE(loaded.map.height_grid.height == 2);
  REQUIRE(loaded.map.height_grid.ground_y.size() == 4);
  REQUIRE(loaded.map.height_grid.ground_y[0] == Catch::Approx(1.0f));
  REQUIRE(loaded.map.height_grid.ground_y[3] == Catch::Approx(4.0f));
  REQUIRE(loaded.map.ramps.size() == 1);
  REQUIRE(loaded.map.ramps[0].tile.x == -1);
  REQUIRE(loaded.map.ramps[0].tile.z == 4);
  REQUIRE(loaded.map.ramps[0].direction == rat::RampDirection::East);
  REQUIRE(loaded.map.ramps[0].low_y == Catch::Approx(2.0f));
  REQUIRE(loaded.map.ramps[0].high_y == Catch::Approx(3.5f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);

  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.schema_version == 2);
  REQUIRE(again.map.height_grid.origin_x == -2);
  REQUIRE(again.map.height_grid.origin_z == 3);
  REQUIRE(again.map.height_grid.width == 2);
  REQUIRE(again.map.height_grid.height == 2);
  REQUIRE(again.map.height_grid.ground_y == loaded.map.height_grid.ground_y);
  REQUIRE(again.map.ramps.size() == 1);
  REQUIRE(again.map.ramps[0].tile.x == -1);
  REQUIRE(again.map.ramps[0].tile.z == 4);
  REQUIRE(again.map.ramps[0].direction == rat::RampDirection::East);
  REQUIRE(again.map.ramps[0].low_y == Catch::Approx(2.0f));
  REQUIRE(again.map.ramps[0].high_y == Catch::Approx(3.5f));
}

TEST_CASE("Map loader rejects invalid height grid length", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "broken",
    "width": 3,
    "height": 3,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 1.0, 2.0]
    },
    "events": []
  })";

  const auto result = rat::load_map_from_string(kJson);
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("Map loader rejects ramp with high below low", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "broken_ramp",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "ramps": [
      { "tile": { "x": 0, "z": 0 }, "direction": "north", "low_y": 3.0, "high_y": 1.0 }
    ],
    "events": []
  })";

  const auto result = rat::load_map_from_string(kJson);
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("Serializer keeps v1 payload free of elevation fields", "[unit][map]") {
  rat::MapData map;
  map.schema_version = 1;
  map.id = "legacy_save";
  map.width = 2;
  map.height = 2;
  map.height_grid.origin_x = 9;
  map.height_grid.origin_z = 9;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {7.0f};
  map.ramps.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::North,
      .low_y = 0.0f,
      .high_y = 1.0f,
  });

  const auto serialized = rat::serialize_map_to_string(map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"height_grid\"") == std::string::npos);
  REQUIRE(serialized.json_text.find("\"ramps\"") == std::string::npos);
}

TEST_CASE("Serializer writes elevation fields for v2", "[unit][map]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "elevated";
  map.width = 2;
  map.height = 2;
  map.height_grid.origin_x = -3;
  map.height_grid.origin_z = 5;
  map.height_grid.width = 2;
  map.height_grid.height = 2;
  map.height_grid.ground_y = {1.0f, 2.0f, 3.0f, 4.0f};
  map.ramps.push_back({
      .tile = rat::TileCoord{-2, 5},
      .direction = rat::RampDirection::West,
      .low_y = 4.0f,
      .high_y = 6.0f,
  });

  const auto serialized = rat::serialize_map_to_string(map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"height_grid\"") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"ramps\"") != std::string::npos);
}

TEST_CASE("Legacy blocker remains full wall and roundtrips without vertical keys", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "legacy_blocker",
    "width": 2,
    "height": 2,
    "blockers": [
      { "min_x": 0.0, "min_z": 0.0, "max_x": 1.0, "max_z": 1.0 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.blockers.size() == 1);
  REQUIRE_FALSE(loaded.map.blockers[0].jumpable);
  REQUIRE_FALSE(loaded.map.blockers[0].base_y.has_value());
  REQUIRE_FALSE(loaded.map.blockers[0].top_y.has_value());
  REQUIRE(loaded.map.blockers[0].bounds.min_x == Catch::Approx(0.0f));
  REQUIRE(loaded.map.blockers[0].bounds.max_x == Catch::Approx(1.0f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"jumpable\"") == std::string::npos);
  REQUIRE(serialized.json_text.find("\"base_y\"") == std::string::npos);
  REQUIRE(serialized.json_text.find("\"top_y\"") == std::string::npos);
}

TEST_CASE("Low jumpable blocker parses and roundtrips with vertical keys", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "low_blocker",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "blockers": [
      {
        "min_x": 0.0,
        "min_z": 0.0,
        "max_x": 1.0,
        "max_z": 1.0,
        "base_y": 0.0,
        "top_y": 0.75,
        "jumpable": true
      }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.blockers.size() == 1);
  REQUIRE(loaded.map.blockers[0].jumpable);
  REQUIRE(loaded.map.blockers[0].base_y.has_value());
  REQUIRE(loaded.map.blockers[0].top_y.has_value());
  REQUIRE(*loaded.map.blockers[0].base_y == Catch::Approx(0.0f));
  REQUIRE(*loaded.map.blockers[0].top_y == Catch::Approx(0.75f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"jumpable\": true") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"base_y\": 0.0") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"top_y\": 0.75") != std::string::npos);
}

TEST_CASE("Loader rejects jumpable blocker missing vertical pair", "[unit][map]") {
  constexpr const char* kMissingTop = R"({
    "schema_version": 1,
    "id": "bad_missing_top",
    "width": 2,
    "height": 2,
    "blockers": [
      {
        "min_x": 0.0,
        "min_z": 0.0,
        "max_x": 1.0,
        "max_z": 1.0,
        "base_y": 0.0,
        "jumpable": true
      }
    ],
    "events": []
  })";
  const auto missing_top = rat::load_map_from_string(kMissingTop);
  REQUIRE_FALSE(missing_top.ok);

  constexpr const char* kMissingBase = R"({
    "schema_version": 1,
    "id": "bad_missing_base",
    "width": 2,
    "height": 2,
    "blockers": [
      {
        "min_x": 0.0,
        "min_z": 0.0,
        "max_x": 1.0,
        "max_z": 1.0,
        "top_y": 1.0,
        "jumpable": true
      }
    ],
    "events": []
  })";
  const auto missing_base = rat::load_map_from_string(kMissingBase);
  REQUIRE_FALSE(missing_base.ok);
}

TEST_CASE("Loader rejects blocker with top below base", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "bad_range",
    "width": 2,
    "height": 2,
    "blockers": [
      {
        "min_x": 0.0,
        "min_z": 0.0,
        "max_x": 1.0,
        "max_z": 1.0,
        "base_y": 2.0,
        "top_y": 1.0,
        "jumpable": false
      }
    ],
    "events": []
  })";
  const auto result = rat::load_map_from_string(kJson);
  REQUIRE_FALSE(result.ok);
}

TEST_CASE("Loader rejects partial vertical fields when non-jumpable", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "bad_partial_vertical",
    "width": 2,
    "height": 2,
    "blockers": [
      {
        "min_x": 0.0,
        "min_z": 0.0,
        "max_x": 1.0,
        "max_z": 1.0,
        "base_y": 0.0,
        "jumpable": false
      }
    ],
    "events": []
  })";
  const auto result = rat::load_map_from_string(kJson);
  REQUIRE_FALSE(result.ok);
}

TEST_CASE("Map loader v2 parses edge_barriers and roundtrips", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "edge_map",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "edge_barriers": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 0.45 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 2);
  REQUIRE(loaded.map.edge_barriers.size() == 1);
  REQUIRE(loaded.map.edge_barriers[0].tile.x == 0);
  REQUIRE(loaded.map.edge_barriers[0].tile.z == 0);
  REQUIRE(loaded.map.edge_barriers[0].direction == rat::RampDirection::East);
  REQUIRE(loaded.map.edge_barriers[0].height == Catch::Approx(0.45f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"edge_barriers\"") != std::string::npos);

  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.edge_barriers.size() == 1);
  REQUIRE(again.map.edge_barriers[0].tile.x == 0);
  REQUIRE(again.map.edge_barriers[0].tile.z == 0);
  REQUIRE(again.map.edge_barriers[0].direction == rat::RampDirection::East);
  REQUIRE(again.map.edge_barriers[0].height == Catch::Approx(0.45f));
}

TEST_CASE("Map loader edge_barriers last wins on tile and direction", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "edge_dup",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "edge_barriers": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 0.45 },
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 1.6 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.edge_barriers.size() == 1);
  REQUIRE(loaded.map.edge_barriers[0].height == Catch::Approx(1.6f));
}

TEST_CASE("Map loader drops edge_barriers on ramp tiles", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "edge_ramp",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "ramps": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "low_y": 0.0, "high_y": 1.0 }
    ],
    "edge_barriers": [
      { "tile": { "x": 0, "z": 0 }, "direction": "north", "height": 0.45 },
      { "tile": { "x": 1, "z": 0 }, "direction": "west", "height": 0.45 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.edge_barriers.size() == 1);
  REQUIRE(loaded.map.edge_barriers[0].tile.x == 1);
  REQUIRE(loaded.map.edge_barriers[0].tile.z == 0);
  REQUIRE(loaded.map.edge_barriers[0].direction == rat::RampDirection::West);
}

TEST_CASE("Map loader drops out-of-grid and non-positive edge_barriers", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "edge_drop",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "edge_barriers": [
      { "tile": { "x": 99, "z": 0 }, "direction": "east", "height": 0.45 },
      { "tile": { "x": 0, "z": 0 }, "direction": "south", "height": 0.0 },
      { "tile": { "x": 0, "z": 1 }, "direction": "west", "height": -1.0 },
      { "tile": { "x": 1, "z": 1 }, "direction": "north", "height": 0.45 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.edge_barriers.size() == 1);
  REQUIRE(loaded.map.edge_barriers[0].tile.x == 1);
  REQUIRE(loaded.map.edge_barriers[0].tile.z == 1);
  REQUIRE(loaded.map.edge_barriers[0].direction == rat::RampDirection::North);
  REQUIRE(loaded.map.edge_barriers[0].height == Catch::Approx(0.45f));
}

TEST_CASE("Map loader keeps opposite-encoding edge_barriers as separate edges", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "edge_opposite",
    "width": 2,
    "height": 1,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 1,
      "ground_y": [0.0, 0.0]
    },
    "edge_barriers": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 0.45 },
      { "tile": { "x": 1, "z": 0 }, "direction": "west", "height": 1.6 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.edge_barriers.size() == 2);
  REQUIRE(loaded.map.edge_barriers[0].tile.x == 0);
  REQUIRE(loaded.map.edge_barriers[0].direction == rat::RampDirection::East);
  REQUIRE(loaded.map.edge_barriers[0].height == Catch::Approx(0.45f));
  REQUIRE(loaded.map.edge_barriers[1].tile.x == 1);
  REQUIRE(loaded.map.edge_barriers[1].direction == rat::RampDirection::West);
  REQUIRE(loaded.map.edge_barriers[1].height == Catch::Approx(1.6f));
}

TEST_CASE("Map loader v1 ignores edge_barriers key", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "legacy_edges",
    "width": 2,
    "height": 2,
    "edge_barriers": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 0.45 }
    ],
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 1);
  REQUIRE(loaded.map.edge_barriers.empty());
}

TEST_CASE("Serializer keeps v1 payload free of edge_barriers", "[unit][map]") {
  rat::MapData map;
  map.schema_version = 1;
  map.id = "legacy_edges_save";
  map.width = 2;
  map.height = 2;
  map.edge_barriers.push_back({
      .tile = rat::TileCoord{0, 0},
      .direction = rat::RampDirection::East,
      .height = 0.45f,
  });

  const auto serialized = rat::serialize_map_to_string(map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"edge_barriers\"") == std::string::npos);
}

TEST_CASE("Map loader v2 omits floor_slabs and ladders", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "v2_empty_slabs",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.floor_slabs.empty());
  REQUIRE(loaded.map.ladders.empty());
}

TEST_CASE("Map loader v3 roundtrips floor slab and ladder", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "house",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 2, "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "floor_slabs": [
      { "tile": { "x": 1, "z": 2 }, "top_y": 2.0, "thickness": 0.25 }
    ],
    "ladders": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "y_lo": 0.0, "y_hi": 2.0 }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 3);
  REQUIRE(loaded.map.floor_slabs.size() == 1);
  REQUIRE(loaded.map.floor_slabs[0].tile.x == 1);
  REQUIRE(loaded.map.floor_slabs[0].tile.z == 2);
  REQUIRE(loaded.map.floor_slabs[0].top_y == Catch::Approx(2.0f));
  REQUIRE(loaded.map.floor_slabs[0].thickness == Catch::Approx(0.25f));
  REQUIRE(loaded.map.ladders.size() == 1);
  REQUIRE(loaded.map.ladders[0].direction == rat::RampDirection::East);
  REQUIRE(loaded.map.ladders[0].y_hi == Catch::Approx(2.0f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.floor_slabs.size() == 1);
  REQUIRE(again.map.ladders[0].tile.x == 0);
}

TEST_CASE("v3 slab missing thickness defaults to 0.25", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "thin",
    "width": 1, "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "floor_slabs": [ { "tile": { "x": 0, "z": 0 }, "top_y": 1.6 } ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.floor_slabs[0].thickness == Catch::Approx(rat::kDefaultFloorSlabThickness));
}

TEST_CASE("Map loader v2 ignores floor_slabs and ladders keys", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "v2_ignore_slabs",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "floor_slabs": [ { "tile": { "x": 0, "z": 0 }, "top_y": 2.0, "thickness": 0.25 } ],
    "ladders": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "y_lo": 0.0, "y_hi": 2.0 }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.floor_slabs.empty());
  REQUIRE(loaded.map.ladders.empty());
}

TEST_CASE("Cyrillic show_text and comment round-trip through serialize",
          "[unit][map][graph][event]") {
  // UTF-8 code units (not source-charset literals) so MSVC without /utf-8 cannot mangle them.
  // Привет / Комментарий
  const std::string privet{"\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"};
  const std::string comment{
      "\xD0\x9A\xD0\xBE\xD0\xBC\xD0\xBC\xD0\xB5\xD0\xBD\xD1\x82\xD0\xB0\xD1\x80\xD0\xB8\xD0\xB9"};

  const std::string json = std::string(R"({
    "schema_version": 1,
    "id": "cyrillic",
    "width": 2,
    "height": 2,
    "events": [{
      "id": "npc",
      "tile": {"x": 0, "z": 0},
      "pages": [{
        "trigger": "action",
        "graph": {
          "nodes": [
            {"id": "say", "kind": "show_text", "params": {"text": ")") +
                           privet + R"("}},
            {"id": "note", "kind": "comment", "params": {"text": ")" + comment + R"("}}
          ],
          "edges": [
            {"from": "entry", "to": "say"},
            {"from": "say", "to": "note"},
            {"from": "note", "to": "exit"}
          ]
        }
      }]
    }]
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(json);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events.size() == 1);
  REQUIRE(loaded.map.events[0].pages.size() == 1);
  const rat::EventPage& page = loaded.map.events[0].pages[0];
  REQUIRE(page.graph.has_value());
  REQUIRE(page.graph->nodes.size() == 2);
  REQUIRE(page.graph->nodes[0].kind == "show_text");
  REQUIRE(page.graph->nodes[0].text == privet);
  REQUIRE(page.graph->nodes[1].kind == "comment");
  REQUIRE(page.graph->nodes[1].text == comment);

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  // Prefer readable UTF-8 in dump (nlohmann dump(2) default ensure_ascii=false).
  REQUIRE(serialized.json_text.find(privet) != std::string::npos);
  REQUIRE(serialized.json_text.find(comment) != std::string::npos);

  const rat::MapLoadResult again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events[0].pages[0].graph.has_value());
  REQUIRE(again.map.events[0].pages[0].graph->nodes[0].text == privet);
  REQUIRE(again.map.events[0].pages[0].graph->nodes[1].text == comment);
}

TEST_CASE("v3 missing slab and ladder arrays loads empty", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "v3_empty",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 3);
  REQUIRE(loaded.map.floor_slabs.empty());
  REQUIRE(loaded.map.ladders.empty());
  REQUIRE(loaded.map.indoor_volumes.empty());
}

TEST_CASE("Map loader v3 ignores indoor_volumes key", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "v3_ignore_indoor",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "indoor_volumes": [
      { "min_x": 0.0, "min_z": 0.0, "max_x": 2.0, "max_z": 2.0, "y_lo": 0.0, "y_hi": 2.5 }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.indoor_volumes.empty());
}

TEST_CASE("Map loader v4 roundtrips indoor volume AABB", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 4,
    "id": "house",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 2, "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "indoor_volumes": [
      { "min_x": 0.0, "min_z": 1.0, "max_x": 3.0, "max_z": 4.0, "y_lo": 0.5, "y_hi": 2.5 }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 4);
  REQUIRE(loaded.map.indoor_volumes.size() == 1);
  REQUIRE(loaded.map.indoor_volumes[0].xz.min_x == Catch::Approx(0.0f));
  REQUIRE(loaded.map.indoor_volumes[0].xz.min_z == Catch::Approx(1.0f));
  REQUIRE(loaded.map.indoor_volumes[0].xz.max_x == Catch::Approx(3.0f));
  REQUIRE(loaded.map.indoor_volumes[0].xz.max_z == Catch::Approx(4.0f));
  REQUIRE(loaded.map.indoor_volumes[0].y_lo == Catch::Approx(0.5f));
  REQUIRE(loaded.map.indoor_volumes[0].y_hi == Catch::Approx(2.5f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"indoor_volumes\"") != std::string::npos);
  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.schema_version == 4);
  REQUIRE(again.map.indoor_volumes.size() == 1);
  REQUIRE(again.map.indoor_volumes[0].xz.max_z == Catch::Approx(4.0f));
  REQUIRE(again.map.indoor_volumes[0].y_hi == Catch::Approx(2.5f));
}

TEST_CASE("Map loader v4 missing indoor_volumes loads empty array", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 4,
    "id": "v4_empty",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 4);
  REQUIRE(loaded.map.indoor_volumes.empty());

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"indoor_volumes\"") != std::string::npos);
}

TEST_CASE("Map loader v5 roundtrips occupancy solid and ramp", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 5,
    "id": "vox",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 2, "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "occupancy": [
      { "x": 0, "y": 1, "z": 0, "kind": "solid" },
      { "x": 1, "y": 0, "z": 0, "kind": "ramp", "yaw": "east" }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 5);
  REQUIRE(loaded.map.occupancy.size() == 2);
  REQUIRE(loaded.map.occupancy[0].x == 0);
  REQUIRE(loaded.map.occupancy[0].y == 1);
  REQUIRE(loaded.map.occupancy[0].z == 0);
  REQUIRE(loaded.map.occupancy[0].kind == rat::OccupancyKind::Solid);
  REQUIRE(loaded.map.occupancy[1].x == 1);
  REQUIRE(loaded.map.occupancy[1].y == 0);
  REQUIRE(loaded.map.occupancy[1].z == 0);
  REQUIRE(loaded.map.occupancy[1].kind == rat::OccupancyKind::Ramp);
  REQUIRE(loaded.map.occupancy[1].yaw == rat::RampDirection::East);

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"occupancy\"") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"kind\"") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"yaw\"") != std::string::npos);
  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.schema_version == 5);
  REQUIRE(again.map.occupancy.size() == 2);
  REQUIRE(again.map.occupancy[0].y == 1);
  REQUIRE(again.map.occupancy[0].kind == rat::OccupancyKind::Solid);
  REQUIRE(again.map.occupancy[1].kind == rat::OccupancyKind::Ramp);
  REQUIRE(again.map.occupancy[1].yaw == rat::RampDirection::East);
}

TEST_CASE("Map loader v5 missing occupancy loads and dumps empty array", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 5,
    "id": "v5_empty",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 5);
  REQUIRE(loaded.map.occupancy.empty());

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"occupancy\"") != std::string::npos);
}

TEST_CASE("Map loader v4 ignores occupancy key", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 4,
    "id": "v4_occ",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "occupancy": [
      { "x": 0, "y": 0, "z": 0, "kind": "solid" }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.occupancy.empty());
}

TEST_CASE("Map loader v4 dump omits occupancy", "[unit][map]") {
  rat::MapData map;
  map.schema_version = 4;
  map.id = "v4_dump_occ";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  map.occupancy.push_back(rat::OccupancyCell{
      .x = 0,
      .y = 0,
      .z = 0,
      .kind = rat::OccupancyKind::Solid,
  });

  const auto serialized = rat::serialize_map_to_string(map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("occupancy") == std::string::npos);
}

TEST_CASE("Map loader rejects unknown occupancy kind", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 5,
    "id": "bad_kind",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "occupancy": [
      { "x": 0, "y": 0, "z": 0, "kind": "glass" }
    ]
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE_FALSE(loaded.ok);
  REQUIRE(loaded.error.find("kind") != std::string::npos);
}

TEST_CASE("Map loader occupancy last-wins on duplicate x y z", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 5,
    "id": "dup_cell",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "occupancy": [
      { "x": 0, "y": 2, "z": 0, "kind": "solid" },
      { "x": 0, "y": 2, "z": 0, "kind": "ramp", "yaw": "south" }
    ]
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.occupancy.size() == 1);
  REQUIRE(loaded.map.occupancy[0].kind == rat::OccupancyKind::Ramp);
  REQUIRE(loaded.map.occupancy[0].yaw == rat::RampDirection::South);
}

TEST_CASE("Map loader occupancy ramp requires yaw", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 5,
    "id": "ramp_no_yaw",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "occupancy": [
      { "x": 0, "y": 0, "z": 0, "kind": "ramp" }
    ]
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE_FALSE(loaded.ok);
  REQUIRE_FALSE(loaded.error.empty());
}

TEST_CASE("Map loader occupancy solid ignores yaw", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 5,
    "id": "solid_yaw",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "occupancy": [
      { "x": 0, "y": 0, "z": 0, "kind": "solid", "yaw": "west" }
    ]
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.occupancy.size() == 1);
  REQUIRE(loaded.map.occupancy[0].kind == rat::OccupancyKind::Solid);
}

TEST_CASE("Map loader v3 dump omits indoor_volumes", "[unit][map]") {
  rat::MapData map;
  map.schema_version = 3;
  map.id = "v3_dump";
  map.width = 1;
  map.height = 1;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  map.indoor_volumes.push_back(rat::IndoorVolume{
      .xz = {0.0f, 0.0f, 1.0f, 1.0f},
      .y_lo = 0.0f,
      .y_hi = 2.0f,
  });

  const auto serialized = rat::serialize_map_to_string(map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("indoor_volumes") == std::string::npos);
}
