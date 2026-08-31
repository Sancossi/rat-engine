#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/hot_apply.hpp>
#include <rat/player.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using Catch::Approx;

namespace {

constexpr const char* kMapA = R"({
  "schema_version": 1,
  "id": "map_a",
  "width": 4,
  "height": 4,
  "tile_size": 1.0,
  "blockers": [{"min_x": 1, "min_z": 1, "max_x": 2, "max_z": 2}],
  "events": [
    {
      "id": "npc_a",
      "tile": { "x": 0, "z": 1 },
      "pages": [{ "trigger": "action", "commands": [{ "op": "show_text", "text": "A" }] }]
    }
  ]
})";

constexpr const char* kMapB = R"({
  "schema_version": 1,
  "id": "map_b",
  "width": 8,
  "height": 8,
  "tile_size": 1.0,
  "blockers": [
    {"min_x": 3, "min_z": -1, "max_x": 5, "max_z": 1},
    {"min_x": -2, "min_z": -2, "max_x": -1, "max_z": -1}
  ],
  "events": [
    {
      "id": "npc_b",
      "tile": { "x": 2, "z": 3 },
      "pages": [{ "trigger": "action", "commands": [{ "op": "show_text", "text": "B" }] }]
    }
  ]
})";

}  // namespace

TEST_CASE("Hot-apply replaces blockers events and map id", "[unit][hot_apply]") {
  rat::EventRuntime events;
  rat::GameState state;
  rat::PlayerBody player;
  player.x = 4.5f;
  player.z = -2.0f;
  std::vector<rat::Aabb2> blockers;
  std::vector<rat::Vec3> markers;

  rat::HotApplyTargets targets{events, state, player, blockers, markers};
  const auto first = rat::hot_apply_map_from_string(kMapA, targets, {.preserve_player_position = true});
  REQUIRE(first.ok);
  REQUIRE(state.map_id() == "map_a");
  REQUIRE(events.map().events.size() == 1);
  REQUIRE(events.map().events[0].id == "npc_a");
  REQUIRE(blockers.size() == 1);
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].x == Approx(0.0f));
  REQUIRE(markers[0].z == Approx(1.0f));
  REQUIRE(player.x == Approx(4.5f));
  REQUIRE(player.z == Approx(-2.0f));

  const auto second = rat::hot_apply_map_from_string(kMapB, targets, {.preserve_player_position = true});
  REQUIRE(second.ok);
  REQUIRE(state.map_id() == "map_b");
  REQUIRE(events.map().events[0].id == "npc_b");
  REQUIRE(blockers.size() == 2);
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].x == Approx(2.0f));
  REQUIRE(markers[0].z == Approx(3.0f));
  REQUIRE(player.x == Approx(4.5f));
  REQUIRE(player.z == Approx(-2.0f));
}

TEST_CASE("Hot-apply can reset player position", "[unit][hot_apply]") {
  rat::EventRuntime events;
  rat::GameState state;
  rat::PlayerBody player;
  player.x = 9.0f;
  player.y = 1.0f;
  player.z = 8.0f;
  std::vector<rat::Aabb2> blockers;
  std::vector<rat::Vec3> markers;

  rat::HotApplyTargets targets{events, state, player, blockers, markers};
  const auto result =
      rat::hot_apply_map_from_string(kMapA, targets, {.preserve_player_position = false});
  REQUIRE(result.ok);
  REQUIRE(player.x == Approx(0.0f));
  REQUIRE(player.y == Approx(0.0f));
  REQUIRE(player.z == Approx(0.0f));
  REQUIRE(state.player_x() == Approx(0.0f));
}

TEST_CASE("Hot-apply failed parse leaves session untouched", "[unit][hot_apply]") {
  rat::EventRuntime events;
  rat::GameState state;
  rat::PlayerBody player;
  std::vector<rat::Aabb2> blockers{{0, 0, 1, 1}};
  std::vector<rat::Vec3> markers{{1, 0, 1}};

  rat::HotApplyTargets targets{events, state, player, blockers, markers};
  REQUIRE(rat::hot_apply_map_from_string(kMapA, targets).ok);

  const auto failed = rat::hot_apply_map_from_string(R"({"schema_version":99,"id":"bad"})", targets);
  REQUIRE_FALSE(failed.ok);
  REQUIRE_FALSE(failed.error.empty());
  REQUIRE(state.map_id() == "map_a");
  REQUIRE(events.map().id == "map_a");
  REQUIRE(blockers.size() == 1);
  REQUIRE(markers.size() == 1);
}

TEST_CASE("Hot-apply grey_yard from file", "[unit][hot_apply]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  rat::EventRuntime events;
  rat::GameState state;
  rat::PlayerBody player;
  player.x = 1.0f;
  player.z = 2.0f;
  std::vector<rat::Aabb2> blockers;
  std::vector<rat::Vec3> markers;

  rat::HotApplyTargets targets{events, state, player, blockers, markers};
  const std::string path = std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json";
  const auto result = rat::hot_apply_map_from_file(path, targets, {.preserve_player_position = true});
  REQUIRE(result.ok);
  REQUIRE(state.map_id() == "grey_yard");
  REQUIRE_FALSE(blockers.empty());
  REQUIRE(events.map().events.size() >= 2);
  REQUIRE(player.x == Approx(1.0f));
  REQUIRE(player.z == Approx(2.0f));
}
