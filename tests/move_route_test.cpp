#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/hot_apply.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <string>

using Catch::Approx;

namespace {

constexpr float kTickDt = 1.0f / 60.0f;

int frames_to_cross_tile(const rat::PlayerBody& player, float tile_size = 1.0f) {
  const float speed = player.speed > 0.0f ? player.speed : 5.0f;
  return static_cast<int>(std::ceil(tile_size / (speed * kTickDt) - 1.0e-6f));
}

void tick(rat::EventRuntime& runtime, rat::GameState& state, const rat::PlayerBody& player,
          int frames = 1) {
  for (int i = 0; i < frames; ++i) {
    runtime.update(state, player, false, kTickDt);
  }
}

constexpr const char* kRouteMap = R"({
  "schema_version": 1,
  "id": "route",
  "width": 8,
  "height": 8,
  "events": [
    {
      "id": "npc",
      "tile": { "x": 1, "z": 2 },
      "pages": [
        {
          "trigger": "autorun",
          "commands": [
            {
              "op": "set_move_route",
              "through": false,
              "route": [
                { "op": "move", "dir": "east" },
                { "op": "move", "dir": "east" },
                { "op": "wait", "frames": 2 },
                { "op": "turn", "dir": "south" }
              ]
            },
            { "op": "control_switch", "id": 1, "value": true }
          ]
        }
      ]
    }
  ]
})";

}  // namespace

TEST_CASE("set_move_route parses nested route and round-trips", "[unit][route]") {
  const auto loaded = rat::load_map_from_string(kRouteMap);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events.size() == 1);
  REQUIRE(loaded.map.events[0].pages[0].commands.size() == 2);

  const rat::Command& cmd = loaded.map.events[0].pages[0].commands[0];
  REQUIRE(cmd.op == rat::CommandOp::SetMoveRoute);
  REQUIRE_FALSE(cmd.through);
  REQUIRE(cmd.then_commands.empty());
  REQUIRE(cmd.route.size() == 4);
  REQUIRE(cmd.route[0].op == rat::RouteStepOp::Move);
  REQUIRE(cmd.route[0].dir == rat::RampDirection::East);
  REQUIRE(cmd.route[1].op == rat::RouteStepOp::Move);
  REQUIRE(cmd.route[1].dir == rat::RampDirection::East);
  REQUIRE(cmd.route[2].op == rat::RouteStepOp::Wait);
  REQUIRE(cmd.route[2].frames == 2);
  REQUIRE(cmd.route[3].op == rat::RouteStepOp::Turn);
  REQUIRE(cmd.route[3].dir == rat::RampDirection::South);

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"set_move_route\"") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"route\"") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"through\"") != std::string::npos);

  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events[0].pages[0].commands[0].op == rat::CommandOp::SetMoveRoute);
  REQUIRE(again.map.events[0].pages[0].commands[0].route.size() == 4);
  REQUIRE(again.map.events[0].pages[0].commands[0].route[2].frames == 2);
  REQUIRE(again.map.schema_version == 1);
}

TEST_CASE("page-level op move does not become a page CommandOp", "[unit][route]") {
  constexpr const char* kPageMove = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "bad",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [ { "op": "move", "dir": "east" } ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kPageMove);
  REQUIRE_FALSE(loaded.ok);
  REQUIRE(loaded.error.find("move") != std::string::npos);
}

TEST_CASE("move step interpolates xz at player speed instead of snapping tile", "[unit][route]") {
  const auto loaded = rat::load_map_from_string(kRouteMap);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  tick(runtime, state, player);

  const auto after_one = runtime.event_overlay("npc");
  REQUIRE(after_one.has_value());
  REQUIRE(after_one->tile.x == 1);
  REQUIRE(after_one->tile.z == 2);
  REQUIRE(after_one->x == Approx(1.5f + player.speed * kTickDt));
  REQUIRE(after_one->z == Approx(2.5f));
  REQUIRE(runtime.map().events[0].tile->x == 1);

  tick(runtime, state, player, frames_to_cross_tile(player) - 1);
  const auto arrived = runtime.event_overlay("npc");
  REQUIRE(arrived.has_value());
  REQUIRE(arrived->tile.x == 2);
  REQUIRE(arrived->tile.z == 2);
  REQUIRE(arrived->x == Approx(2.5f));
  REQUIRE(arrived->z == Approx(2.5f));
}

TEST_CASE("command_index stays on set_move_route while route runs", "[unit][route]") {
  const auto loaded = rat::load_map_from_string(kRouteMap);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{});

  const auto debug = runtime.foreground_debug();
  REQUIRE(debug.has_value());
  REQUIRE(debug->command_index == 0);
  REQUIRE(debug->route_index == 0);
  REQUIRE_FALSE(state.get_switch(1));

  tick(runtime, state, rat::PlayerBody{}, frames_to_cross_tile(rat::PlayerBody{}) * 2 + 8);
  REQUIRE(state.get_switch(1));
  REQUIRE_FALSE(runtime.player_input_blocked());
}

TEST_CASE("foreground occupancy by player does not block; blocker without through waits",
          "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "fg",
    "width": 8,
    "height": 8,
    "blockers": [
      { "min_x": 2.0, "min_z": 0.0, "max_x": 3.0, "max_z": 1.0 }
    ],
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              {
                "op": "set_move_route",
                "through": false,
                "route": [
                  { "op": "move", "dir": "east" },
                  { "op": "move", "dir": "east" }
                ]
              }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 0.5f;

  tick(runtime, state, player);
  auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 0);
  REQUIRE(runtime.player_input_blocked());

  tick(runtime, state, player, frames_to_cross_tile(player));
  overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 1);
  REQUIRE(runtime.player_input_blocked());

  tick(runtime, state, player, 4);
  overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 1);
}

TEST_CASE("through ignores blocker and completes the step", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "thru",
    "width": 8,
    "height": 8,
    "blockers": [
      { "min_x": 1.0, "min_z": 0.0, "max_x": 2.0, "max_z": 1.0 }
    ],
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              {
                "op": "set_move_route",
                "through": true,
                "route": [ { "op": "move", "dir": "east" } ]
              }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{}, frames_to_cross_tile(rat::PlayerBody{}));

  const auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 1);
}

TEST_CASE("parallel waits a frame when dest is occupied and does not lock the player",
          "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "par",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "parallel",
            "commands": [
              {
                "op": "set_move_route",
                "route": [ { "op": "move", "dir": "east" } ]
              }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 0.5f;

  tick(runtime, state, player);
  REQUIRE_FALSE(runtime.player_input_blocked());
  auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 0);

  tick(runtime, state, player);
  overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 0);

  player.x = 3.5f;
  tick(runtime, state, player, frames_to_cross_tile(player));
  overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 1);
}

TEST_CASE("started parallel lerp finishes even if dest becomes occupied", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "par_inflight",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "parallel",
            "commands": [
              { "op": "set_move_route", "route": [ { "op": "move", "dir": "east" } ] }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 3.5f;
  player.z = 0.5f;
  tick(runtime, state, player);
  auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 0);
  REQUIRE(overlay->x > 0.5f);

  player.x = 1.5f;
  tick(runtime, state, player, frames_to_cross_tile(player));
  overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 1);
  REQUIRE(overlay->x == Approx(1.5f));
}

TEST_CASE("overlay-caused overlap does not start PlayerTouch", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "touch",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              {
                "op": "set_move_route",
                "route": [ { "op": "move", "dir": "east" } ]
              },
              { "op": "control_switch", "id": 1, "value": true }
            ]
          },
          {
            "trigger": "player_touch",
            "conditions": [ { "type": "switch", "id": 1, "value": true } ],
            "commands": [ { "op": "control_switch", "id": 2, "value": true } ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 0.5f;

  tick(runtime, state, player, frames_to_cross_tile(player) + 2);
  REQUIRE(state.get_switch(1));
  REQUIRE_FALSE(state.get_switch(2));
  REQUIRE_FALSE(runtime.player_input_blocked());
}

TEST_CASE("player walking into live overlay bounds can still start PlayerTouch", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "walk_in",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              {
                "op": "set_move_route",
                "route": [ { "op": "move", "dir": "east" }, { "op": "move", "dir": "east" } ]
              },
              { "op": "control_switch", "id": 1, "value": true }
            ]
          },
          {
            "trigger": "player_touch",
            "conditions": [ { "type": "switch", "id": 1, "value": true } ],
            "commands": [ { "op": "control_switch", "id": 2, "value": true } ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody away;
  away.x = 6.5f;
  away.z = 0.5f;
  tick(runtime, state, away, frames_to_cross_tile(away) * 2 + 2);
  REQUIRE(state.get_switch(1));
  REQUIRE_FALSE(state.get_switch(2));

  rat::PlayerBody on_live;
  on_live.x = 2.5f;
  on_live.z = 0.5f;
  tick(runtime, state, on_live);
  REQUIRE(state.get_switch(2));
}

TEST_CASE("tile plus volume live bounds shift like translate_event_on_grid", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "vol",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 1, "z": 1 },
        "volume": { "min_x": 1.0, "min_z": 1.0, "max_x": 2.0, "max_z": 2.0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "set_move_route", "route": [ { "op": "move", "dir": "east" } ] }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody at_spawn;
  at_spawn.x = 1.5f;
  at_spawn.z = 1.5f;
  tick(runtime, state, at_spawn, frames_to_cross_tile(at_spawn));

  const auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 2);

  REQUIRE(runtime.map().events[0].volume->min_x == Approx(1.0f));

  rat::PlayerBody at_shifted;
  at_shifted.x = 2.5f;
  at_shifted.z = 1.5f;
  REQUIRE_FALSE(runtime.player_overlaps(runtime.map().events[0], at_spawn));
  REQUIRE(runtime.player_overlaps(runtime.map().events[0], at_shifted));
}

TEST_CASE("volume-only set_move_route skips and warns", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "vol_only",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "zone",
        "volume": { "min_x": 0.0, "min_z": 0.0, "max_x": 1.0, "max_z": 1.0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "set_move_route", "route": [ { "op": "move", "dir": "east" } ] },
              { "op": "control_switch", "id": 3, "value": true }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{}, 2);

  REQUIRE_FALSE(runtime.event_overlay("zone").has_value());
  REQUIRE(state.get_switch(3));
  REQUIRE_FALSE(runtime.warnings().empty());
  const bool warned = std::any_of(
      runtime.warnings().begin(), runtime.warnings().end(), [](const std::string& w) {
        return w.find("volume") != std::string::npos;
      });
  REQUIRE(warned);
}

TEST_CASE("parallel budget counts set_move_route as one opcode like Wait", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "budget",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "parallel",
            "commands": [
              {
                "op": "set_move_route",
                "route": [
                  { "op": "move", "dir": "east" },
                  { "op": "move", "dir": "east" },
                  { "op": "move", "dir": "east" }
                ]
              }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{});

  REQUIRE(runtime.last_parallel_commands_executed() == 1);
  const auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 0);

  tick(runtime, state, rat::PlayerBody{}, frames_to_cross_tile(rat::PlayerBody{}));
  const auto arrived = runtime.event_overlay("npc");
  REQUIRE(arrived.has_value());
  REQUIRE(arrived->tile.x == 1);
}

TEST_CASE("markers follow live overlay xz", "[unit][route]") {
  const auto loaded = rat::load_map_from_string(kRouteMap);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  rat::PlayerBody player;
  tick(runtime, state, player);

  const auto mid = runtime.event_markers();
  REQUIRE(mid.size() == 1);
  REQUIRE(mid[0].x == Approx(1.5f + player.speed * kTickDt));
  REQUIRE(mid[0].z == Approx(2.5f));

  tick(runtime, state, player, frames_to_cross_tile(player) - 1);
  const auto markers = runtime.event_markers();
  REQUIRE(markers.size() == 1);
  REQUIRE(markers[0].x == Approx(2.5f));
  REQUIRE(markers[0].z == Approx(2.5f));

  const auto authored = rat::event_markers_from_map(runtime.map());
  REQUIRE(authored[0].x == Approx(1.5f));
}

TEST_CASE("load resets overlay; save blob does not keep patrol", "[unit][route]") {
  const auto loaded = rat::load_map_from_string(kRouteMap);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{});
  REQUIRE(runtime.event_overlay("npc").has_value());

  std::string blob;
  REQUIRE(state.save_to_memory(blob));
  REQUIRE(blob.find("npc") == std::string::npos);

  REQUIRE(runtime.load(loaded.map).ok);
  REQUIRE_FALSE(runtime.event_overlay("npc").has_value());
}

TEST_CASE("hot-apply resets overlay", "[unit][route]") {
  const auto loaded = rat::load_map_from_string(kRouteMap);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  rat::PlayerBody player;
  std::vector<rat::BlockerDef> blockers;
  std::vector<rat::Vec3> markers;
  rat::HotApplyTargets targets{runtime, state, player, blockers, markers};

  REQUIRE(rat::hot_apply_map(loaded.map, targets).ok);
  tick(runtime, state, player);
  REQUIRE(runtime.event_overlay("npc").has_value());

  REQUIRE(rat::hot_apply_map(loaded.map, targets).ok);
  REQUIRE_FALSE(runtime.event_overlay("npc").has_value());
}

TEST_CASE("edge fence without through blocks dest cell", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "fence",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0]
    },
    "edge_barriers": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 0.45 }
    ],
    "events": [
      {
        "id": "npc",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "set_move_route", "route": [ { "op": "move", "dir": "east" } ] }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{}, 3);

  const auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == 0);
}

TEST_CASE("set_move_route steps onto height_grid tiles west of world origin", "[unit][route]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "neg_origin",
    "width": 16,
    "height": 16,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": -4,
      "origin_z": -3,
      "width": 4,
      "height": 4,
      "ground_y": [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0]
    },
    "events": [
      {
        "id": "npc",
        "tile": { "x": -4, "z": -1 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "set_move_route", "through": false, "route": [ { "op": "move", "dir": "east" } ] }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);
  tick(runtime, state, rat::PlayerBody{}, frames_to_cross_tile(rat::PlayerBody{}));

  const auto overlay = runtime.event_overlay("npc");
  REQUIRE(overlay.has_value());
  REQUIRE(overlay->tile.x == -3);
  REQUIRE(overlay->tile.z == -1);
}
