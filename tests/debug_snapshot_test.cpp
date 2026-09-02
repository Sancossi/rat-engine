#include <rat/app_mode.hpp>
#include <rat/asset.hpp>
#include <rat/audio.hpp>
#include <rat/clock.hpp>
#include <rat/collision.hpp>
#include <rat/debug_snapshot.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/render_world.hpp>
#include <rat/simulation_session.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <system_error>

TEST_CASE("write_debug_snapshot round-trips a headless fixture", "[unit][debug]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "snap",
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
  state.set_switch(3, true);
  state.set_variable(2, 9);
  state.add_item("rusty_cog", 1, true);

  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;
  runtime.update(state, player, false, 1.0f / 60.0f);

  rat::JumpState jump = rat::make_grounded_jump_state();
  jump.jump_offset = 1.25f;
  jump.grounded = false;
  jump.ladder_lockout_left = 0.15f;
  jump.ladder_bounce_x = -1.0f;
  jump.ladder_bounce_z = 0.0f;
  jump.climbing = true;
  jump.climb_into_x = 1.0f;
  jump.climb_into_z = 0.0f;

  const rat::DebugSnapshot written = rat::make_debug_snapshot(
      42, rat::AppMode::Play, player, jump, runtime, state);

  const auto path = std::filesystem::temp_directory_path() / "rat-debug-snapshot-test.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_debug_snapshot(path.string(), written));

  const auto read = rat::read_debug_snapshot(path.string());
  REQUIRE(read.has_value());
  CHECK(read->sim_frame == 42);
  CHECK(read->app_mode == "PLAY");
  CHECK(read->player_x == 0.5f);
  CHECK(read->player_y == 0.0f);
  CHECK(read->player_z == 0.5f);
  CHECK(read->jump.jump_offset == 1.25f);
  CHECK_FALSE(read->jump.grounded);
  CHECK(read->jump.ladder_lockout_left == 0.15f);
  CHECK(read->jump.ladder_bounce_x == -1.0f);
  CHECK(read->jump.ladder_bounce_z == 0.0f);
  CHECK(read->jump.climbing);
  CHECK(read->jump.climb_into_x == 1.0f);
  CHECK(read->jump.climb_into_z == 0.0f);
  REQUIRE_FALSE(read->overlapping_event_ids.empty());
  CHECK(read->overlapping_event_ids[0] == "intro");
  REQUIRE(read->active_interpreter.has_value());
  CHECK(read->active_interpreter->event_id == "intro");
  CHECK(read->active_interpreter->page_index == 0);
  CHECK(read->active_interpreter->waiting_message);
  CHECK(read->switches.at(3) == true);
  CHECK(read->variables.at(2) == 9);
  REQUIRE(read->items.size() == 1);
  CHECK(read->items[0].id == "rusty_cog");
  REQUIRE(read->active_message.has_value());
  CHECK(*read->active_message == "Hello");
  REQUIRE(read->event_why_not.size() == 1);
  CHECK(read->event_why_not[0].id == "intro");
  CHECK(read->event_why_not[0].reason == "already_running");
  CHECK(read->event_why_not_reason == "already_running");

  std::filesystem::remove(path, ec);
}

TEST_CASE("write_debug_snapshot grounded jump defaults lockout and bounce to zero",
          "[unit][debug]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "snap_lockout_defaults",
    "width": 2,
    "height": 2,
    "events": []
  })";

  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 0.5f;
  player.y = 0.0f;
  player.z = 0.5f;

  const rat::JumpState jump = rat::make_grounded_jump_state();
  const rat::DebugSnapshot written =
      rat::make_debug_snapshot(1, rat::AppMode::Play, player, jump, runtime, state);

  const auto path =
      std::filesystem::temp_directory_path() / "rat-debug-snapshot-lockout-defaults.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_debug_snapshot(path.string(), written));

  const auto read = rat::read_debug_snapshot(path.string());
  REQUIRE(read.has_value());
  REQUIRE(read->jump.ladder_lockout_left == 0.0f);
  REQUIRE(read->jump.ladder_bounce_x == 0.0f);
  REQUIRE(read->jump.ladder_bounce_z == 0.0f);
  REQUIRE(read->jump.climb_into_x == 0.0f);
  REQUIRE(read->jump.climb_into_z == 0.0f);

  std::filesystem::remove(path, ec);
}

TEST_CASE("make_debug_snapshot primary why-not follows selected_event_id", "[unit][debug][why]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "snap_selected_why",
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
  REQUIRE(runtime.load(loaded.map).ok);

  rat::PlayerBody player;
  player.x = 2.5f;
  player.y = 0.0f;
  player.z = 2.5f;

  const rat::DebugSnapshot fallback = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true);
  REQUIRE(fallback.event_why_not.size() == 2);
  CHECK(fallback.event_why_not[0].id == "sign");
  CHECK(fallback.event_why_not[0].reason == "height");
  CHECK(fallback.event_why_not[1].id == "gated");
  CHECK(fallback.event_why_not[1].reason == "conditions");
  CHECK(fallback.event_why_not_reason == "height");

  const rat::DebugSnapshot selected = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true, "gated");
  REQUIRE(selected.event_why_not.size() == 2);
  CHECK(selected.event_why_not[0].id == "sign");
  CHECK(selected.event_why_not[1].id == "gated");
  CHECK(selected.event_why_not_reason == "conditions");
  CHECK(selected.event_why_not_reason != fallback.event_why_not_reason);

  const rat::DebugSnapshot unknown = rat::make_debug_snapshot(
      1, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, true, "missing");
  CHECK(unknown.event_why_not_reason == fallback.event_why_not_reason);
  REQUIRE(unknown.event_why_not.size() == 2);
}

TEST_CASE("write_debug_snapshot round-trips numeric frame metrics", "[unit][debug][metrics]") {
  rat::EventRuntime runtime;
  rat::GameState state;
  rat::PlayerBody player;

  rat::FrameMetrics metrics;
  metrics.simulation_tick_seconds = 0.008;
  metrics.event_commands = 3;
  metrics.draw_calls = 5;
  metrics.transient_bytes = 128;
  metrics.transient_allocations = 2;
  metrics.asset_uploads = 1;
  metrics.audio_queue_depth = 4;
  metrics.audio_overflow_count = 7;
  metrics.collision_candidates = 9;

  const rat::DebugSnapshot written = rat::make_debug_snapshot(
      7, rat::AppMode::Play, player, rat::make_grounded_jump_state(), runtime, state, false, {}, {},
      0, metrics);

  const auto path = std::filesystem::temp_directory_path() / "rat-debug-metrics-test.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_debug_snapshot(path.string(), written));

  const auto raw = rat::os_files().read(path.string());
  REQUIRE(raw.ok);
  const nlohmann::json root = nlohmann::json::parse(raw.bytes.as_text());
  REQUIRE(root.contains("metrics"));
  REQUIRE(root["metrics"].is_object());
  const nlohmann::json& node = root["metrics"];
  CHECK(node["simulation_tick_seconds"].is_number());
  CHECK(node["event_commands"].is_number());
  CHECK(node["draw_calls"].is_number());
  CHECK(node["transient_bytes"].is_number());
  CHECK(node["transient_allocations"].is_number());
  CHECK(node["asset_uploads"].is_number());
  CHECK(node["audio_queue_depth"].is_number());
  CHECK(node["audio_overflow_count"].is_number());
  CHECK(node["collision_candidates"].is_number());

  const auto read = rat::read_debug_snapshot(path.string());
  REQUIRE(read.has_value());
  CHECK(read->metrics.simulation_tick_seconds == Catch::Approx(0.008));
  CHECK(read->metrics.event_commands == 3);
  CHECK(read->metrics.draw_calls == 5);
  CHECK(read->metrics.transient_bytes == 128);
  CHECK(read->metrics.transient_allocations == 2);
  CHECK(read->metrics.asset_uploads == 1);
  CHECK(read->metrics.audio_queue_depth == 4);
  CHECK(read->metrics.audio_overflow_count == 7);
  CHECK(read->metrics.collision_candidates == 9);

  std::filesystem::remove(path, ec);
}

TEST_CASE("collect_frame_metrics fills snapshot JSON from FakeClock and live systems",
          "[unit][debug][metrics]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "metrics_live";
  map.width = 2;
  map.height = 1;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 2;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f, 1.0f};

  rat::EventDef autorun;
  autorun.id = "boot";
  autorun.tile = rat::TileCoord{0, 0};
  rat::EventPage page;
  page.trigger = rat::TriggerKind::Autorun;
  rat::Command sw;
  sw.op = rat::CommandOp::ControlSwitch;
  sw.id = 1;
  sw.bool_value = true;
  rat::Command var;
  var.op = rat::CommandOp::ControlVariable;
  var.id = 0;
  var.int_value = 4;
  page.commands.push_back(sw);
  page.commands.push_back(var);
  autorun.pages.push_back(page);
  map.events.push_back(autorun);

  rat::FakeClock clock;
  clock.set_auto_advance_seconds(0.004);
  rat::SimulationSession session;
  session.set_clock(&clock);
  REQUIRE(session.load(map).ok);
  session.tick({});

  const rat::RenderWorld world = rat::capture_render_world(session);
  rat::FrameAllocator frame;
  (void)frame.allocate(16);
  (void)frame.allocate(8);

  rat::MemoryAssetLoader loader;
  const rat::AssetId tex = rat::make_asset_id("tex/metrics");
  rat::AssetCpuData cpu;
  cpu.bytes = {1, 2};
  loader.set(tex, cpu);
  rat::AssetRegistry assets(loader);
  rat::AssetCatalogEntry entry;
  entry.id = tex;
  entry.kind = rat::AssetKind::Texture;
  assets.register_asset(entry);
  assets.request_load(tex);
  assets.pump_loads();

  rat::RecordingAudioSink sink;
  rat::QueuedAudio audio(sink);
  audio.play_sfx("a");
  audio.play_sfx("b");
  audio.play_sfx("c");

  rat::FrameMetricsSources sources;
  sources.session = &session;
  sources.render = &world;
  sources.frame = &frame;
  sources.assets = &assets;
  sources.audio = &audio;
  const rat::FrameMetrics metrics = rat::collect_frame_metrics(sources);

  CHECK(metrics.simulation_tick_seconds == Catch::Approx(0.004));
  CHECK(metrics.event_commands == 2);
  CHECK(metrics.draw_calls == static_cast<int>(world.packets.size()));
  CHECK(metrics.draw_calls >= 1);
  CHECK(metrics.transient_bytes == frame.used());
  CHECK(metrics.transient_allocations == 2);
  CHECK(metrics.asset_uploads == 1);
  CHECK(metrics.audio_queue_depth == 3);
  CHECK(metrics.audio_overflow_count == 0);
  const rat::CollisionWorld collision =
      rat::bake_collision_world(session.events().map(), *session.surface_query());
  CHECK(metrics.collision_candidates == static_cast<int>(collision.fences.size()));
  CHECK(metrics.collision_candidates >= 1);

  const rat::DebugSnapshot snapshot = rat::make_debug_snapshot(
      session.tick_id(), rat::AppMode::Play, session.player(), session.jump(), session.events(),
      session.state(), false, {}, {}, 0, metrics);

  const auto path = std::filesystem::temp_directory_path() / "rat-debug-metrics-live.json";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  REQUIRE(rat::write_debug_snapshot(path.string(), snapshot));
  const auto raw = rat::os_files().read(path.string());
  REQUIRE(raw.ok);
  const nlohmann::json root = nlohmann::json::parse(raw.bytes.as_text());
  REQUIRE(root["metrics"].is_object());
  for (const char* key : {"simulation_tick_seconds", "event_commands", "draw_calls",
                          "transient_bytes", "transient_allocations", "asset_uploads",
                          "audio_queue_depth", "audio_overflow_count", "collision_candidates"}) {
    REQUIRE(root["metrics"].contains(key));
    CHECK(root["metrics"][key].is_number());
  }

  std::filesystem::remove(path, ec);
}
