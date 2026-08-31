#include <rat/blocker_edit.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
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
  const auto result = rat::load_map_from_string(R"({"schema_version":2,"id":"x","width":1,"height":1})");
  REQUIRE_FALSE(result.ok);
  REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("Example grey_yard.json loads without crash", "[unit][map]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined for map file tests
#endif
  const std::string path = std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json";
  const auto result = rat::load_map_from_file(path);
  REQUIRE(result.ok);
  REQUIRE(result.map.id == "grey_yard");
  REQUIRE_FALSE(result.map.blockers.empty());
  REQUIRE(result.map.events.size() >= 2);

  bool found_autorun = false;
  bool found_branchish = false;
  for (const auto& ev : result.map.events) {
    for (const auto& page : ev.pages) {
      if (page.trigger == rat::TriggerKind::Autorun) {
        found_autorun = true;
      }
      if (!page.conditions.empty()) {
        found_branchish = true;
      }
    }
  }
  REQUIRE(found_autorun);
  REQUIRE(found_branchish);

  // Round-trip via string path also works.
  const auto from_string = rat::load_map_from_string(read_file(path));
  REQUIRE(from_string.ok);
  REQUIRE(from_string.map.events.size() == result.map.events.size());
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
  loaded.map.blockers[0] = rat::translate_aabb_on_grid(loaded.map.blockers[0], 2, 0, 1.0f);

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);

  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.blockers.size() == 1);
  REQUIRE(again.map.blockers[0].min_x == Catch::Approx(2.0f));
  REQUIRE(again.map.blockers[0].max_x == Catch::Approx(3.0f));
}

TEST_CASE("Save map file roundtrips blocker and event edits", "[unit][map]") {
  rat::MapData map;
  map.id = "saved";
  map.width = 8;
  map.height = 8;
  map.blockers.push_back({2.0f, 3.0f, 4.0f, 5.0f});

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
  REQUIRE(loaded.map.blockers[0].min_x == Catch::Approx(2.0f));
  REQUIRE(loaded.map.events.size() == 1);
  REQUIRE(loaded.map.events[0].tile->x == 6);
  REQUIRE(loaded.map.events[0].tile->z == -2);
}
