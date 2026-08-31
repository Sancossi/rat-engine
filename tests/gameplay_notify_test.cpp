#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/gameplay_notify.hpp>
#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace {

rat::MapData make_flat_surface_map() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "notify_land";
  map.width = 1;
  map.height = 1;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f};
  return map;
}

}  // namespace

TEST_CASE("GameplayNotifyBus posts subscribed handlers in FIFO order", "[unit][notify]") {
  rat::GameplayNotifyBus bus;
  std::vector<rat::GameplayNotify> recorded;
  bus.subscribe([&recorded](const rat::GameplayNotify& notify) { recorded.push_back(notify); });

  bus.post({rat::GameplayNotifyKind::ItemPicked, "cog"});
  bus.post({rat::GameplayNotifyKind::DialogShown, {}});

  REQUIRE(recorded.size() == 2);
  CHECK(recorded[0].kind == rat::GameplayNotifyKind::ItemPicked);
  CHECK(recorded[0].id == "cog");
  CHECK(recorded[1].kind == rat::GameplayNotifyKind::DialogShown);
  CHECK(recorded[1].id.empty());
}

TEST_CASE("Autorun ShowText then ChangeItems posts both notifies and still mutates GameState",
          "[unit][notify]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "loot",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "show_text", "text": "Got it" },
              { "op": "change_items", "id": "rusty_cog", "delta": 1 }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameplayNotifyBus bus;
  std::vector<rat::GameplayNotify> recorded;
  bus.subscribe([&recorded](const rat::GameplayNotify& notify) { recorded.push_back(notify); });

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.set_notify(&bus);

  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "Got it");
  REQUIRE(recorded.size() == 1);
  CHECK(recorded[0].kind == rat::GameplayNotifyKind::DialogShown);

  runtime.acknowledge_message();
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(state.has_item("rusty_cog"));
  REQUIRE(state.item_quantity("rusty_cog") == 1);
  REQUIRE(recorded.size() == 2);
  CHECK(recorded[1].kind == rat::GameplayNotifyKind::ItemPicked);
  CHECK(recorded[1].id == "rusty_cog");
}

TEST_CASE("ChangeItems with negative delta does not post ItemPicked", "[unit][notify]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "spend",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "change_items", "id": "rusty_cog", "delta": -1 }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameplayNotifyBus bus;
  std::vector<rat::GameplayNotify> recorded;
  bus.subscribe([&recorded](const rat::GameplayNotify& notify) { recorded.push_back(notify); });

  rat::GameState state;
  state.add_item("rusty_cog", 1);
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.set_notify(&bus);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(state.item_quantity("rusty_cog") == 0);
  REQUIRE(recorded.empty());
}

TEST_CASE("ShowText with null notify still shows message", "[unit][notify]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "talk",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "show_text", "text": "Hello" }
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
  runtime.set_notify(nullptr);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(runtime.active_message() == "Hello");
}

TEST_CASE("PlayerFrameResult.landed is true only on the airborne to grounded frame",
          "[unit][notify]") {
  const rat::MapData map = make_flat_surface_map();
  const rat::SurfaceQuery query(map);

  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.speed = 0.0f;

  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::GameplayNotifyBus bus;
  std::vector<rat::GameplayNotifyKind> kinds;
  bus.subscribe([&kinds](const rat::GameplayNotify& notify) { kinds.push_back(notify.kind); });

  int landing_frame = -1;
  constexpr float dt = 1.0f / 120.0f;
  constexpr int kMaxFrames = 600;
  for (int frame = 0; frame < kMaxFrames; ++frame) {
    rat::PlayerFrameInput input;
    input.jump_pressed = frame == 0;
    input.jump_held = frame <= 8;
    const rat::PlayerFrameResult result =
        rat::integrate_player_frame_surface(body, jump, input, dt, {}, query);
    body = result.body;
    jump = result.jump;

    if (frame == 0) {
      REQUIRE_FALSE(result.landed);
      REQUIRE_FALSE(jump.grounded);
      continue;
    }

    if (result.landed) {
      REQUIRE(jump.grounded);
      REQUIRE(landing_frame < 0);
      landing_frame = frame;
      bus.post({rat::GameplayNotifyKind::Landed, {}});
      break;
    }

    REQUIRE_FALSE(result.landed);
    REQUIRE_FALSE(jump.grounded);
  }

  REQUIRE(landing_frame > 0);
  REQUIRE(jump.grounded);
  REQUIRE(kinds.size() == 1);
  CHECK(kinds[0] == rat::GameplayNotifyKind::Landed);

  for (int i = 0; i < 8; ++i) {
    const rat::PlayerFrameResult grounded =
        rat::integrate_player_frame_surface(body, jump, {}, dt, {}, query);
    body = grounded.body;
    jump = grounded.jump;
    REQUIRE_FALSE(grounded.landed);
    REQUIRE(jump.grounded);
  }
}
