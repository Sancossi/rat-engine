#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/hot_apply.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
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

constexpr const char* kHeightMap = R"({
  "schema_version": 2,
  "id": "height_map",
  "width": 6,
  "height": 4,
  "tile_size": 1.0,
  "height_grid": {
    "origin_x": 0,
    "origin_z": 0,
    "width": 6,
    "height": 4,
    "ground_y": [
      1, 1, 1, 1, 1, 1,
      1, 2, 2, 5, 2, 1,
      1, 2, 2, 2, 2, 1,
      1, 1, 1, 1, 1, 1
    ]
  },
  "events": [
    {
      "id": "tile_hi",
      "tile": { "x": 1, "z": 1 },
      "pages": [{ "trigger": "action", "commands": [{ "op": "comment", "text": "tile" }] }]
    },
    {
      "id": "volume_hi",
      "volume": { "min_x": 2.0, "min_z": 0.0, "max_x": 4.0, "max_z": 2.0 },
      "pages": [{ "trigger": "player_touch", "commands": [{ "op": "comment", "text": "vol" }] }]
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
  std::vector<rat::BlockerDef> blockers;
  std::vector<rat::Vec3> markers;

  rat::HotApplyTargets targets{events, state, player, blockers, markers};
  const auto first = rat::hot_apply_map_from_string(kMapA, targets, {.preserve_player_position = true});
  REQUIRE(first.ok);
  REQUIRE(state.map_id() == "map_a");
  REQUIRE(events.map().events.size() == 1);
  REQUIRE(events.map().events[0].id == "npc_a");
  REQUIRE(blockers.size() == 1);
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].x == Approx(0.5f));
  REQUIRE(markers[0].z == Approx(1.5f));
  REQUIRE(player.x == Approx(4.5f));
  REQUIRE(player.z == Approx(-2.0f));

  const auto second = rat::hot_apply_map_from_string(kMapB, targets, {.preserve_player_position = true});
  REQUIRE(second.ok);
  REQUIRE(state.map_id() == "map_b");
  REQUIRE(events.map().events[0].id == "npc_b");
  REQUIRE(blockers.size() == 2);
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].x == Approx(2.5f));
  REQUIRE(markers[0].z == Approx(3.5f));
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
  std::vector<rat::BlockerDef> blockers;
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
  std::vector<rat::BlockerDef> blockers{{.bounds = {0, 0, 1, 1}}};
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
  std::vector<rat::BlockerDef> blockers;
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

TEST_CASE("Event markers sample elevated tile and volume centers", "[unit][hot_apply]") {
  const auto loaded = rat::load_map_from_string(kHeightMap);
  REQUIRE(loaded.ok);

  const auto markers = rat::event_markers_from_map(loaded.map);
  REQUIRE(markers.size() == 2);
  REQUIRE(markers[0].x == Approx(1.5f));
  REQUIRE(markers[0].z == Approx(1.5f));
  REQUIRE(markers[0].y == Approx(2.0f));
  REQUIRE(markers[1].x == Approx(3.0f));
  REQUIRE(markers[1].z == Approx(1.0f));
  REQUIRE(markers[1].y == Approx(5.0f));
}

TEST_CASE("Event markers sit on EventDef bind Y not ground sample", "[unit][hot_apply]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "loft_marker",
    "width": 2,
    "height": 2,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 2,
      "height": 2,
      "ground_y": [0, 0, 0, 0]
    },
    "floor_slabs": [
      { "tile": { "x": 0, "z": 0 }, "top_y": 2.0, "thickness": 0.25 }
    ],
    "events": [
      {
        "id": "loft_plank",
        "tile": { "x": 0, "z": 0 },
        "y": 2.0,
        "pages": [{ "trigger": "action", "commands": [{ "op": "comment", "text": "loft" }] }]
      },
      {
        "id": "ground_npc",
        "tile": { "x": 1, "z": 0 },
        "pages": [{ "trigger": "action", "commands": [{ "op": "comment", "text": "ground" }] }]
      },
      {
        "id": "loft_volume",
        "volume": { "min_x": 0.0, "min_z": 1.0, "max_x": 1.0, "max_z": 2.0 },
        "y": 1.5,
        "pages": [{ "trigger": "player_touch", "commands": [{ "op": "comment", "text": "vol" }] }]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  const auto markers = rat::event_markers_from_map(loaded.map);
  REQUIRE(markers.size() == 3);
  REQUIRE(markers[0].x == Approx(0.5f));
  REQUIRE(markers[0].z == Approx(0.5f));
  REQUIRE(markers[0].y == Approx(2.0f));
  REQUIRE(markers[1].x == Approx(1.5f));
  REQUIRE(markers[1].z == Approx(0.5f));
  REQUIRE(markers[1].y == Approx(0.0f));
  REQUIRE(markers[2].x == Approx(0.5f));
  REQUIRE(markers[2].z == Approx(1.5f));
  REQUIRE(markers[2].y == Approx(1.5f));
}

TEST_CASE("Hot-apply refreshes cached query and resets jump state", "[unit][hot_apply]") {
  const auto loaded = rat::load_map_from_string(kHeightMap);
  REQUIRE(loaded.ok);

  rat::EventRuntime events;
  rat::GameState state;
  rat::PlayerBody player;
  player.x = 1.5f;
  player.y = 42.0f;
  player.z = 1.5f;
  rat::JumpState jump;
  jump.grounded = false;
  jump.jump_offset = 1.5f;
  jump.vertical_speed = -4.0f;
  jump.jump_buffer_left = 0.08f;
  jump.coyote_time_left = 0.02f;

  std::vector<rat::BlockerDef> blockers;
  std::vector<rat::Vec3> markers;
  std::unique_ptr<rat::SurfaceQuery> cache;
  rat::HotApplyTargets targets{events, state, player, blockers, markers, &cache, &jump};

  const auto applied =
      rat::hot_apply_map_from_string(kHeightMap, targets, {.preserve_player_position = true});
  REQUIRE(applied.ok);
  REQUIRE(cache != nullptr);
  REQUIRE(player.x == Approx(1.5f));
  REQUIRE(player.z == Approx(1.5f));
  REQUIRE(player.y == Approx(2.0f));
  REQUIRE(state.player_y() == Approx(2.0f));
  REQUIRE(jump.grounded);
  REQUIRE(jump.jump_offset == Approx(0.0f));
  REQUIRE(jump.vertical_speed == Approx(0.0f));
  REQUIRE(jump.jump_buffer_left == Approx(0.0f));
  REQUIRE(jump.coyote_time_left == Approx(0.0f));
  REQUIRE(cache->sample(3.0f, 1.0f).y == Approx(5.0f));
}
