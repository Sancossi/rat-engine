#include <rat/clock.hpp>
#include <rat/file_store.hpp>
#include <rat/gameplay_notify.hpp>
#include <rat/input.hpp>
#include <rat/input_sequence.hpp>
#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/save_game.hpp>
#include <rat/simulation_session.hpp>
#include <rat/surface_query.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

using Catch::Approx;

namespace {

rat::MapData make_flat_map() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "sim_flat";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);
  return map;
}

rat::PlayerBody make_start_player() {
  rat::PlayerBody player;
  player.x = 1.5f;
  player.y = 0.0f;
  player.z = 1.5f;
  player.speed = 5.0f;
  return player;
}

}  // namespace

TEST_CASE("SimulationSession tick advances tick_id and integrates player", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());

  REQUIRE(session.tick_id() == 0);

  rat::InputFrame frame;
  frame.move = rat::world_aligned_move(0.0f, 1.0f);
  const rat::SimulationTickResult result = session.tick(frame);

  CHECK(result.tick_id == 1);
  CHECK(session.tick_id() == 1);
  CHECK(session.player().z < 1.5f);
  CHECK(session.state().player_z() == Approx(session.player().z).margin(1e-5f));
  CHECK(session.state().map_id() == "sim_flat");
}

TEST_CASE("drain_simulation_catch_up matches repeated ticks independent of display FPS",
          "[unit][sim]") {
  const rat::MapData map = make_flat_map();
  const rat::PlayerBody start = make_start_player();
  rat::InputFrame frame;
  frame.move = rat::world_aligned_move(0.0f, 1.0f);

  rat::SimulationSession stepped;
  REQUIRE(stepped.load(map).ok);
  stepped.set_player(start);
  constexpr int kTicks = 12;
  for (int i = 0; i < kTicks; ++i) {
    stepped.tick(frame);
  }

  rat::SimulationSession hitch;
  REQUIRE(hitch.load(map).ok);
  hitch.set_player(start);
  float accumulator = hitch.config().dt * static_cast<float>(kTicks);
  const rat::SimulationCatchUpResult catch_up =
      rat::drain_simulation_catch_up(hitch, accumulator, frame);

  CHECK(catch_up.ticks_run == kTicks);
  CHECK_FALSE(catch_up.budget_exceeded);
  CHECK(accumulator < hitch.config().dt);
  CHECK(hitch.tick_id() == static_cast<std::uint64_t>(kTicks));
  CHECK(hitch.player().x == Approx(stepped.player().x).margin(1e-5f));
  CHECK(hitch.player().y == Approx(stepped.player().y).margin(1e-5f));
  CHECK(hitch.player().z == Approx(stepped.player().z).margin(1e-5f));
}

TEST_CASE("drain_simulation_catch_up diagnoses catch-up budget exceeded", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());

  const float dt = session.config().dt;
  float accumulator = dt * static_cast<float>(rat::kMaxCatchUpTicks + 10);
  rat::InputFrame frame;
  const rat::SimulationCatchUpResult catch_up =
      rat::drain_simulation_catch_up(session, accumulator, frame);

  CHECK(catch_up.budget_exceeded);
  CHECK(catch_up.ticks_run == rat::kMaxCatchUpTicks);
  CHECK(session.tick_id() == static_cast<std::uint64_t>(rat::kMaxCatchUpTicks));
  CHECK(accumulator < dt);
}

TEST_CASE("drain_simulation_catch_up applies jump edge on first tick only", "[unit][sim]") {
  const rat::MapData map = make_flat_map();
  const rat::PlayerBody start = make_start_player();

  rat::InputFrame press;
  press.jump_pressed = true;
  press.jump_held = true;
  rat::SimulationSession hitch;
  REQUIRE(hitch.load(map).ok);
  hitch.set_player(start);
  float accumulator = hitch.config().dt * 3.0f;
  const rat::SimulationCatchUpResult catch_up =
      rat::drain_simulation_catch_up(hitch, accumulator, press);

  rat::SimulationSession once;
  REQUIRE(once.load(map).ok);
  once.set_player(start);
  once.tick(press);
  rat::InputFrame held;
  held.jump_held = true;
  once.tick(held);
  once.tick(held);

  rat::SimulationSession retrigger;
  REQUIRE(retrigger.load(map).ok);
  retrigger.set_player(start);
  for (int i = 0; i < 3; ++i) {
    retrigger.tick(press);
  }

  CHECK(catch_up.ticks_run == 3);
  CHECK(hitch.tick_id() == 3);
  CHECK_FALSE(hitch.jump().grounded);
  CHECK(hitch.jump().jump_buffer_left == Approx(0.0f).margin(1e-6f));
  CHECK(hitch.player().y == Approx(once.player().y).margin(1e-5f));
  CHECK(hitch.jump().vertical_speed == Approx(once.jump().vertical_speed).margin(1e-5f));
  CHECK(retrigger.jump().jump_buffer_left > 0.0f);
}

TEST_CASE("drain_simulation_catch_up latches jump edge across a zero-tick display frame",
          "[unit][sim]") {
  // Editor samples jump as a display-frame edge, then drains. When display FPS > sim rate
  // (or leftover accumulator is short), that frame runs 0 ticks; previous_buttons_ already
  // consumed the edge, so the next drain has jump_pressed == false.
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());

  rat::InputButtons down;
  down.jump = true;
  rat::InputButtons previous{};

  const float display_dt = session.config().dt * 0.5f;
  float accumulator = 0.0f;

  const rat::InputFrame press = rat::map_input_frame(down, previous, {});
  REQUIRE(press.jump_pressed);
  previous = down;
  accumulator += display_dt;
  const rat::SimulationCatchUpResult skipped =
      rat::drain_simulation_catch_up(session, accumulator, press);
  REQUIRE(skipped.ticks_run == 0);
  REQUIRE(session.tick_id() == 0);
  REQUIRE(session.jump().grounded);

  const rat::InputFrame held = rat::map_input_frame(down, previous, {});
  REQUIRE_FALSE(held.jump_pressed);
  REQUIRE(held.jump_held);
  accumulator += display_dt;
  const rat::SimulationCatchUpResult launched =
      rat::drain_simulation_catch_up(session, accumulator, held);

  CHECK(launched.ticks_run == 1);
  CHECK(session.tick_id() == 1);
  CHECK_FALSE(session.jump().grounded);
}

TEST_CASE("drain_simulation_catch_up latches interact edge across a zero-tick display frame",
          "[unit][sim]") {
  // Editor samples interact as a display-frame edge, then drains. When that frame runs 0 ticks,
  // previous_buttons_ already consumed the edge; the next drain has interact_pressed == false.
  // Jump latches via note_jump_pressed(); interact must persist in interact_buffer_ the same way.
  rat::MapData map = make_flat_map();
  rat::EventDef npc;
  npc.id = "npc";
  npc.tile = rat::TileCoord{1, 1};
  rat::EventPage page;
  page.trigger = rat::TriggerKind::Action;
  rat::Command cmd;
  cmd.op = rat::CommandOp::ControlSwitch;
  cmd.id = 5;
  cmd.bool_value = true;
  page.commands.push_back(cmd);
  npc.pages.push_back(page);
  map.events.push_back(npc);

  rat::SimulationSession session;
  REQUIRE(session.load(map).ok);
  session.set_player(make_start_player());
  REQUIRE(session.events().has_action_prompt(session.player(), session.state()));
  REQUIRE_FALSE(session.state().get_switch(5));

  rat::InputButtons down;
  down.interact = true;
  rat::InputButtons previous{};

  const float display_dt = session.config().dt * 0.5f;
  float accumulator = 0.0f;

  const rat::InputFrame press = rat::map_input_frame(down, previous, {});
  REQUIRE(press.interact_pressed);
  previous = down;
  accumulator += display_dt;
  const rat::SimulationCatchUpResult skipped =
      rat::drain_simulation_catch_up(session, accumulator, press);
  REQUIRE(skipped.ticks_run == 0);
  REQUIRE(session.tick_id() == 0);
  REQUIRE_FALSE(session.state().get_switch(5));

  const rat::InputFrame held = rat::map_input_frame(down, previous, {});
  REQUIRE_FALSE(held.interact_pressed);
  accumulator += display_dt;
  const rat::SimulationCatchUpResult fired =
      rat::drain_simulation_catch_up(session, accumulator, held);

  CHECK(fired.ticks_run == 1);
  CHECK(session.tick_id() == 1);
  CHECK(session.state().get_switch(5));
}

TEST_CASE("drain_simulation_catch_up latches interact mount across a zero-tick display frame",
          "[unit][sim]") {
  // Same 0-tick display-frame miss as jump / NPC interact, but Action is consumed by player
  // mount first. EventRuntime must not fire a co-located Action on that tick.
  rat::MapData map;
  map.schema_version = 3;
  map.id = "sim_ladder_mount";
  map.width = 1;
  map.height = 1;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 1;
  map.height_grid.height = 1;
  map.height_grid.ground_y.assign(1, 0.0f);
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});

  rat::EventDef npc;
  npc.id = "npc";
  npc.tile = rat::TileCoord{0, 0};
  rat::EventPage page;
  page.trigger = rat::TriggerKind::Action;
  rat::Command cmd;
  cmd.op = rat::CommandOp::ControlSwitch;
  cmd.id = 5;
  cmd.bool_value = true;
  page.commands.push_back(cmd);
  npc.pages.push_back(page);
  map.events.push_back(npc);

  rat::PlayerBody player;
  player.x = 0.85f;
  player.y = 1.0f;
  player.z = 0.5f;
  player.speed = 5.0f;

  rat::SimulationSession session;
  REQUIRE(session.load(map).ok);
  session.set_player(player);
  REQUIRE(session.events().has_action_prompt(session.player(), session.state()));
  REQUIRE_FALSE(session.state().get_switch(5));
  REQUIRE_FALSE(session.jump().climbing);

  rat::InputButtons down;
  down.interact = true;
  rat::InputButtons previous{};

  const float display_dt = session.config().dt * 0.5f;
  float accumulator = 0.0f;

  const rat::InputFrame press = rat::map_input_frame(down, previous, {});
  REQUIRE(press.interact_pressed);
  previous = down;
  accumulator += display_dt;
  const rat::SimulationCatchUpResult skipped =
      rat::drain_simulation_catch_up(session, accumulator, press);
  REQUIRE(skipped.ticks_run == 0);
  REQUIRE(session.tick_id() == 0);
  REQUIRE_FALSE(session.jump().climbing);
  REQUIRE_FALSE(session.state().get_switch(5));

  const rat::InputFrame held = rat::map_input_frame(down, previous, {});
  REQUIRE_FALSE(held.interact_pressed);
  accumulator += display_dt;
  const rat::SimulationCatchUpResult mounted =
      rat::drain_simulation_catch_up(session, accumulator, held);

  CHECK(mounted.ticks_run == 1);
  CHECK(session.tick_id() == 1);
  CHECK(session.jump().climbing);
  CHECK_FALSE(session.state().get_switch(5));
}

TEST_CASE("clear_pending_input drops jump buffer so keyboard capture cannot launch",
          "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());

  rat::InputFrame jump;
  jump.jump_pressed = true;
  jump.jump_held = true;
  REQUIRE_FALSE(session.tick(jump).landed);
  REQUIRE_FALSE(session.jump().grounded);

  rat::InputFrame buffer;
  buffer.jump_pressed = true;
  session.tick(buffer);
  REQUIRE(session.jump().jump_buffer_left > 0.0f);
  REQUIRE_FALSE(session.jump().grounded);

  // Editor maps WantCaptureKeyboard to a gated InputFrame + this helper.
  session.clear_pending_input();
  REQUIRE(session.jump().jump_buffer_left == Approx(0.0f).margin(1e-6f));

  bool landed = false;
  for (int i = 0; i < 600; ++i) {
    const rat::SimulationTickResult result = session.tick({});
    if (result.landed) {
      landed = true;
      REQUIRE(session.jump().grounded);
      break;
    }
  }
  REQUIRE(landed);
  REQUIRE(session.jump().grounded);
}

TEST_CASE("tick does not drop jump buffer just because jump_pressed is false", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());

  rat::InputFrame jump;
  jump.jump_pressed = true;
  jump.jump_held = true;
  REQUIRE_FALSE(session.tick(jump).landed);

  rat::InputFrame buffer;
  buffer.jump_pressed = true;
  session.tick(buffer);
  const float buffered = session.jump().jump_buffer_left;
  REQUIRE(buffered > 0.0f);

  session.tick({});
  CHECK(session.jump().jump_buffer_left > 0.0f);
  CHECK(session.jump().jump_buffer_left < buffered);
}

TEST_CASE("SimulationSession tick posts Landed on airborne to grounded", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());

  rat::GameplayNotifyBus bus;
  std::vector<rat::GameplayNotifyKind> kinds;
  bus.subscribe([&kinds](const rat::GameplayNotify& notify) { kinds.push_back(notify.kind); });
  session.set_notify(&bus);

  rat::InputFrame jump;
  jump.jump_pressed = true;
  jump.jump_held = true;
  REQUIRE_FALSE(session.tick(jump).landed);
  REQUIRE_FALSE(session.jump().grounded);

  bool saw_land = false;
  rat::InputFrame held;
  held.jump_held = true;
  for (int i = 0; i < 600; ++i) {
    if (i > 8) {
      held.jump_held = false;
    }
    const rat::SimulationTickResult result = session.tick(held);
    if (result.landed) {
      saw_land = true;
      REQUIRE(session.jump().grounded);
      break;
    }
    REQUIRE_FALSE(session.jump().grounded);
  }

  REQUIRE(saw_land);
  REQUIRE(std::find(kinds.begin(), kinds.end(), rat::GameplayNotifyKind::Landed) != kinds.end());
}

TEST_CASE("SimulationSession grey_yard west ramp approach crests the high cell",
          "[unit][sim][surface]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);

  rat::SimulationSession session;
  REQUIRE(session.load(loaded.map).ok);

  rat::PlayerBody start;
  start.x = 7.5f;
  start.z = 8.3f;
  start.y = 0.0f;
  start.speed = 5.0f;
  session.set_player(start);

  session.tick({});
  REQUIRE(session.events().active_message().has_value());
  rat::InputFrame ack;
  ack.interact_pressed = true;
  session.tick(ack);
  session.tick({});
  session.tick({});
  REQUIRE_FALSE(session.events().player_input_blocked());

  rat::InputFrame walk;
  walk.move = {1.0f, 0.0f};
  for (int i = 0; i < 240; ++i) {
    session.tick(walk);
    if (session.player().x > 9.05f && session.player().x < 10.0f &&
        session.player().y > 0.5f) {
      break;
    }
  }

  REQUIRE(session.player().x > 9.0f);
  REQUIRE(session.player().x < 10.0f);
  REQUIRE(session.player().y == Approx(1.0f).margin(0.06f));
  REQUIRE(session.jump().grounded);
}

TEST_CASE("run_input_sequence adapter matches SimulationSession ticks on grey_yard",
          "[probe][sim][mechanics]") {
#ifndef RAT_TEST_DATA_DIR
#error RAT_TEST_DATA_DIR must be defined
#endif
  const auto loaded =
      rat::load_map_from_file(std::string(RAT_TEST_DATA_DIR) + "/maps/grey_yard.json");
  REQUIRE(loaded.ok);

  rat::SurfaceQuery query(loaded.map);
  rat::PlayerBody start;
  start.x = 1.5f;
  start.z = 1.5f;
  start.y = query.sample(start.x, start.z).y;
  start.speed = 5.0f;

  std::vector<rat::InputFrame> steps;
  steps.push_back({});
  rat::InputFrame interact;
  interact.interact_pressed = true;
  steps.push_back(interact);
  steps.push_back({});
  steps.push_back({});
  rat::InputFrame move;
  move.move = rat::world_aligned_move(0.0f, 1.0f);
  for (int i = 0; i < 12; ++i) {
    steps.push_back(move);
  }

  rat::SimulationSession session;
  REQUIRE(session.load(loaded.map).ok);
  session.set_player(start);
  for (const rat::InputFrame& frame : steps) {
    session.tick(frame);
  }

  const rat::InputSequenceResult seq = rat::run_input_sequence(loaded.map, start, steps);
  CHECK(seq.sim_frame == session.tick_id());
  CHECK(seq.player.x == Approx(session.player().x).margin(1e-5f));
  CHECK(seq.player.y == Approx(session.player().y).margin(1e-5f));
  CHECK(seq.player.z == Approx(session.player().z).margin(1e-5f));
  CHECK(seq.state.get_variable(0) == session.state().get_variable(0));
  CHECK(seq.jump.grounded == session.jump().grounded);
}

TEST_CASE("SimulationSession restores switch and player from a FileStore save", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);

  rat::PlayerBody saved_player = make_start_player();
  saved_player.x = 2.5f;
  saved_player.z = 2.25f;
  session.set_player(saved_player);
  session.state().set_switch(5, true);
  session.state().set_variable(2, 11);
  session.state().add_item("rusty_cog", 1, true);

  rat::MemoryFileStore files;
  REQUIRE(rat::save_game(files, "saves/slot1.ratsave", session.state()).ok);

  rat::PlayerBody mutated = make_start_player();
  mutated.x = 0.5f;
  mutated.z = 0.75f;
  session.set_player(mutated);
  session.state().set_switch(5, false);
  session.state().set_variable(2, 0);
  session.state().add_item("potion", 1);

  rat::GameState loaded;
  REQUIRE(rat::load_game(files, "saves/slot1.ratsave", loaded).ok);
  REQUIRE(session.apply_loaded_game(loaded).ok);

  CHECK(session.state().get_switch(5));
  CHECK(session.state().get_variable(2) == 11);
  CHECK(session.state().item_quantity("rusty_cog") == 1);
  CHECK_FALSE(session.state().has_item("potion"));
  CHECK(session.state().map_id() == "sim_flat");
  CHECK(session.player().x == Approx(2.5f).margin(1e-5f));
  CHECK(session.player().z == Approx(2.25f).margin(1e-5f));
  CHECK(session.state().player_x() == Approx(2.5f).margin(1e-5f));
  CHECK(session.state().player_z() == Approx(2.25f).margin(1e-5f));
  CHECK(session.jump().grounded);
}

TEST_CASE("SimulationSession load keeps saved player Y", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);

  rat::PlayerBody saved_player = make_start_player();
  saved_player.y = 1.25f;
  session.set_player(saved_player);

  rat::MemoryFileStore files;
  REQUIRE(rat::save_game(files, "saves/slot1.ratsave", session.state()).ok);

  rat::PlayerBody mutated = session.player();
  mutated.y = 0.0f;
  session.set_player(mutated);

  rat::GameState loaded;
  REQUIRE(rat::load_game(files, "saves/slot1.ratsave", loaded).ok);
  REQUIRE(session.apply_loaded_game(loaded).ok);

  CHECK(session.player().y == Approx(1.25f).margin(1e-5f));
  CHECK(session.state().player_y() == Approx(1.25f).margin(1e-5f));
  CHECK(session.jump().grounded);
}

TEST_CASE("SimulationSession rejects a foreign map_id on load", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);

  const rat::PlayerBody start = make_start_player();
  session.set_player(start);
  session.state().set_switch(5, false);

  rat::GameState foreign;
  foreign.set_map_id("dungeon");
  foreign.set_player_position(9.0f, 3.0f, 8.0f);
  foreign.set_switch(5, true);

  const rat::GameFileResult result = session.apply_loaded_game(foreign);
  REQUIRE_FALSE(result.ok);
  CHECK_FALSE(result.error.empty());

  CHECK(session.player().x == Approx(start.x).margin(1e-5f));
  CHECK(session.player().y == Approx(start.y).margin(1e-5f));
  CHECK(session.player().z == Approx(start.z).margin(1e-5f));
  CHECK_FALSE(session.state().get_switch(5));
  CHECK(session.state().map_id() == "sim_flat");
}

TEST_CASE("SimulationSession rejects a save with empty map_id", "[unit][sim]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);

  rat::PlayerBody saved_player = make_start_player();
  saved_player.x = 2.0f;
  session.set_player(saved_player);

  rat::GameState loaded;
  loaded.set_player_position(3.25f, 1.25f, 0.75f);
  REQUIRE(loaded.map_id().empty());

  REQUIRE_FALSE(session.apply_loaded_game(loaded).ok);
  CHECK(session.player().x == saved_player.x);
  CHECK(session.player().y == saved_player.y);
  CHECK(session.player().z == saved_player.z);
  CHECK(session.state().map_id() == "sim_flat");
}

TEST_CASE("SimulationSession tick duration follows FakeClock auto-advance",
          "[unit][sim][metrics]") {
  rat::FakeClock clock;
  clock.set_auto_advance_seconds(0.004);
  rat::SimulationSession session;
  session.set_clock(&clock);
  REQUIRE(session.load(make_flat_map()).ok);
  session.tick({});
  CHECK(session.last_tick_seconds() == Approx(0.004));
}

TEST_CASE("Apply loaded game rejects invalid public state without changing live session", "[unit][sim][storage]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map()).ok);
  session.set_player(make_start_player());
  session.state().set_variable(7, 42);
  const auto before = session.player();
  for (int fault = 0; fault < 3; ++fault) {
    rat::GameState loaded;
    if (fault == 1) loaded.set_map_id("other");
    if (fault == 2) {
      loaded.set_map_id("sim_flat");
      loaded.set_player_position(std::numeric_limits<float>::quiet_NaN(), 0, 0);
    }
    CHECK_FALSE(session.apply_loaded_game(loaded).ok);
    CHECK(session.state().get_variable(7) == 42);
    CHECK(session.state().map_id() == "sim_flat");
    CHECK(session.player().x == before.x);
    CHECK(session.player().z == before.z);
  }
}
