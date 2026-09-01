#include <rat/app_mode.hpp>
#include <rat/debug_snapshot.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("event_why_not_fired reports height when player elevation differs", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "height_why",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 3, 0,
        0, 0, 0, 0
      ]
    },
    "events": [
      {
        "id": "sign",
        "tile": { "x": 2, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 11, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 2.5f;
  player.z = 2.5f;
  player.y = 0.0f;

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "sign", state, player, true);
  REQUIRE(reason == rat::EventWhyNot::Height);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "height");
}

TEST_CASE("event_why_not_fired reports conditions when enable switch is off", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "switch_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "gated",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "action",
            "conditions": [{ "type": "switch", "id": 1, "value": true }],
            "commands": [{ "op": "control_switch", "id": 2, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "gated", state, player, true);
  REQUIRE(reason == rat::EventWhyNot::Conditions);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "conditions");
}

TEST_CASE("event_why_not_fired reports already_running for live foreground", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "running_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "intro",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "show_text", "text": "Hello" },
              { "op": "control_variable", "id": 0, "value": 1 }
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
  runtime.load(loaded.map);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Hello");

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "intro", state, rat::PlayerBody{}, false);
  REQUIRE(reason == rat::EventWhyNot::AlreadyRunning);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "already_running");
}

TEST_CASE("event_why_not_fired reports ok for startable Action with interact", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "ok_why",
    "width": 8,
    "height": 8,
    "events": [
      {
        "id": "sign",
        "tile": { "x": 2, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 5, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody near;
  near.x = 2.1f;
  near.z = 2.1f;

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "sign", state, near, true);
  REQUIRE(reason == rat::EventWhyNot::Ok);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "ok");
}

TEST_CASE("event_why_not_fired reports autorun_lock after spent autorun", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "lock_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "intro",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [{ "op": "control_switch", "id": 3, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  const rat::EventWhyNot before =
      rat::event_why_not_fired(runtime, "intro", state, rat::PlayerBody{}, false);
  REQUIRE(before == rat::EventWhyNot::Ok);
  REQUIRE(std::string(rat::event_why_not_name(before)) == "ok");

  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(state.get_switch(3));
  REQUIRE_FALSE(runtime.foreground_debug().has_value());

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "intro", state, rat::PlayerBody{}, false);
  REQUIRE(reason != rat::EventWhyNot::Ok);
  REQUIRE(reason == rat::EventWhyNot::AutorunLock);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "autorun_lock");
}

TEST_CASE("event_why_not_fired reports foreground_busy for blocked autorun", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "busy_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "holder",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [{ "op": "show_text", "text": "Busy" }]
          }
        ]
      },
      {
        "id": "queued",
        "tile": { "x": 1, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [{ "op": "control_switch", "id": 4, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Busy");

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "queued", state, rat::PlayerBody{}, false);
  REQUIRE(reason != rat::EventWhyNot::Ok);
  REQUIRE(reason == rat::EventWhyNot::ForegroundBusy);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "foreground_busy");
}

TEST_CASE("event_why_not_fired reports parallel_limit when cap is full", "[unit][why]") {
  std::string json = R"({"schema_version":1,"id":"cap_why","width":8,"height":8,"events":[)";
  for (int i = 0; i < rat::kMaxParallelEvents + 1; ++i) {
    if (i > 0) {
      json += ',';
    }
    json += R"({"id":"p)" + std::to_string(i) + R"(","tile":{"x":)" + std::to_string(i) +
            R"(,"z":0},"pages":[{"trigger":"parallel","commands":[)"
            R"({"op":"wait","frames":1000}]}]})" ;
  }
  json += "]}";

  const auto loaded = rat::load_map_from_string(json);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_parallel_count() == rat::kMaxParallelEvents);

  const std::string extra_id = "p" + std::to_string(rat::kMaxParallelEvents);
  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, extra_id, state, rat::PlayerBody{}, false);
  REQUIRE(reason != rat::EventWhyNot::Ok);
  REQUIRE(reason == rat::EventWhyNot::ParallelLimit);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "parallel_limit");
}

TEST_CASE("event_why_not_fired reports ok for startable Parallel under cap", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "ok_parallel_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "loop",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "parallel",
            "commands": [{ "op": "wait", "frames": 1000 }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  const rat::EventWhyNot reason =
      rat::event_why_not_fired(runtime, "loop", state, rat::PlayerBody{}, false);
  REQUIRE(reason == rat::EventWhyNot::Ok);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "ok");
}

TEST_CASE("event_why_not_fired reports ok for PlayerTouch on rising edge", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "ok_touch_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "pad",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "player_touch",
            "commands": [{ "op": "control_switch", "id": 7, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;

  const rat::EventWhyNot reason = rat::event_why_not_fired(runtime, "pad", state, player, false);
  REQUIRE(reason == rat::EventWhyNot::Ok);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "ok");
}

TEST_CASE("event_why_not_fired reports already_inside after player_touch edge", "[unit][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "touch_why",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "pad",
        "tile": { "x": 1, "z": 1 },
        "pages": [
          {
            "trigger": "player_touch",
            "commands": [{ "op": "control_switch", "id": 7, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 1.5f;
  player.z = 1.5f;

  runtime.update(state, player, false, 1.0f / 60.0f);
  REQUIRE(state.get_switch(7));
  REQUIRE_FALSE(runtime.foreground_debug().has_value());

  const rat::EventWhyNot reason = rat::event_why_not_fired(runtime, "pad", state, player, false);
  REQUIRE(reason != rat::EventWhyNot::Ok);
  REQUIRE(reason == rat::EventWhyNot::AlreadyInside);
  REQUIRE(std::string(rat::event_why_not_name(reason)) == "already_inside");
}

TEST_CASE("make_debug_snapshot lists why-not for every map event", "[unit][why][debug]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "snap_why",
    "width": 4,
    "height": 4,
    "tile_size": 1.0,
    "height_grid": {
      "origin_x": 0,
      "origin_z": 0,
      "width": 4,
      "height": 4,
      "ground_y": [
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 3, 0,
        0, 0, 0, 0
      ]
    },
    "events": [
      {
        "id": "sign",
        "tile": { "x": 2, "z": 2 },
        "pages": [
          {
            "trigger": "action",
            "commands": [{ "op": "control_switch", "id": 11, "value": true }]
          }
        ]
      },
      {
        "id": "gated",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "action",
            "conditions": [{ "type": "switch", "id": 1, "value": true }],
            "commands": [{ "op": "control_switch", "id": 2, "value": true }]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);

  rat::PlayerBody player;
  player.x = 2.5f;
  player.y = 0.0f;
  player.z = 2.5f;

  const rat::DebugSnapshot snapshot = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true);

  REQUIRE(snapshot.event_why_not.size() == 2);
  CHECK(snapshot.event_why_not[0].id == "sign");
  CHECK(snapshot.event_why_not[0].reason == "height");
  CHECK(snapshot.event_why_not[1].id == "gated");
  CHECK(snapshot.event_why_not[1].reason == "conditions");
  CHECK(snapshot.event_why_not_reason == "height");
}
