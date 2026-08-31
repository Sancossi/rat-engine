#include <rat/map_loader.hpp>

#include <catch2/catch_test_macros.hpp>

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
