#include <rat/blocker_edit.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

namespace {

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(in.good());
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
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
}

TEST_CASE("Map loader rejects unknown schema version", "[unit][map]") {
  const auto result = rat::load_map_from_string(R"({"schema_version":3,"id":"x","width":1,"height":1})");
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
  REQUIRE(result.map.schema_version == 2);
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
      REQUIRE(ev.pages[0].commands.size() == 2);
      REQUIRE(ev.pages[0].commands[1].op == rat::CommandOp::ControlSwitch);
      REQUIRE(ev.pages[0].commands[1].id == 40);
      REQUIRE(ev.pages[0].commands[1].bool_value);
      found_elevated_event = true;
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
  REQUIRE(result.map.ramps.size() == 1);
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

  REQUIRE(found_autorun);
  REQUIRE(found_branchish);
  REQUIRE(found_crate_notice);
  REQUIRE(found_jumpable_blocker);
  REQUIRE(found_elevated_event);

  // Round-trip via string path also works.
  const auto from_string = rat::load_map_from_string(read_file(path));
  REQUIRE(from_string.ok);
  REQUIRE(from_string.map.events.size() == result.map.events.size());
  REQUIRE(from_string.map.schema_version == 2);
  REQUIRE(from_string.map.height_grid.ground_y == result.map.height_grid.ground_y);

  const auto serialized = rat::serialize_map_to_string(result.map);
  REQUIRE(serialized.ok);
  const auto from_serialized = rat::load_map_from_string(serialized.json_text);
  REQUIRE(from_serialized.ok);
  REQUIRE(from_serialized.map.schema_version == 2);
  REQUIRE(from_serialized.map.height_grid.origin_x == result.map.height_grid.origin_x);
  REQUIRE(from_serialized.map.height_grid.origin_z == result.map.height_grid.origin_z);
  REQUIRE(from_serialized.map.height_grid.width == result.map.height_grid.width);
  REQUIRE(from_serialized.map.height_grid.height == result.map.height_grid.height);
  REQUIRE(from_serialized.map.height_grid.ground_y == result.map.height_grid.ground_y);
  REQUIRE(from_serialized.map.ramps.size() == result.map.ramps.size());
  REQUIRE(from_serialized.map.ramps[0].tile.x == 8);
  REQUIRE(from_serialized.map.ramps[0].tile.z == 8);
  REQUIRE(from_serialized.map.blockers.size() == result.map.blockers.size());
  REQUIRE(from_serialized.map.events.size() == result.map.events.size());
  REQUIRE(from_serialized.map.events.back().id == "elevated_after_blocker");
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
