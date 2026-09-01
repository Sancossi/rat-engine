#include <rat/game_state.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("GameState switches default to false", "[unit][gamestate]") {
  rat::GameState state;
  REQUIRE_FALSE(state.get_switch(1));
  state.set_switch(1, true);
  REQUIRE(state.get_switch(1));
  state.set_switch(1, false);
  REQUIRE_FALSE(state.get_switch(1));
}

TEST_CASE("GameState variables default to zero", "[unit][gamestate]") {
  rat::GameState state;
  REQUIRE(state.get_variable(7) == 0);
  state.set_variable(7, 42);
  REQUIRE(state.get_variable(7) == 42);
}

TEST_CASE("GameState inventory is RM-like list with quantities", "[unit][gamestate]") {
  rat::GameState state;
  REQUIRE_FALSE(state.has_item("door_key"));
  state.add_item("door_key", 1, true);
  REQUIRE(state.has_item("door_key"));
  REQUIRE(state.item_quantity("door_key") == 1);
  state.add_item("potion", 2, false);
  state.add_item("potion", 1, false);
  REQUIRE(state.item_quantity("potion") == 3);
  REQUIRE(state.inventory().size() == 2);
}

TEST_CASE("GameState clear resets progress fields", "[unit][gamestate]") {
  rat::GameState state;
  state.set_switch(3, true);
  state.set_variable(2, 9);
  state.set_self_switch("npc", 'B', true);
  state.add_item("coin", 5);
  state.set_map_id("yard");
  state.set_player_position(1.0f, 2.0f, 3.0f);
  state.clear();
  REQUIRE_FALSE(state.get_switch(3));
  REQUIRE(state.get_variable(2) == 0);
  REQUIRE_FALSE(state.get_self_switch("npc", 'B'));
  REQUIRE(state.inventory().empty());
  REQUIRE(state.map_id().empty());
  REQUIRE(state.player_x() == 0.0f);
}

TEST_CASE("GameState self-switches A-D are per event", "[unit][gamestate]") {
  rat::GameState state;
  REQUIRE_FALSE(state.get_self_switch("a", 'A'));
  state.set_self_switch("a", 'A', true);
  state.set_self_switch("a", 'C', true);
  state.set_self_switch("b", 'A', true);
  REQUIRE(state.get_self_switch("a", 'A'));
  REQUIRE_FALSE(state.get_self_switch("a", 'B'));
  REQUIRE(state.get_self_switch("a", 'C'));
  REQUIRE(state.get_self_switch("b", 'A'));
  state.set_self_switch("a", 'A', false);
  REQUIRE_FALSE(state.get_self_switch("a", 'A'));
}

TEST_CASE("GameState save/load memory roundtrip", "[unit][gamestate]") {
  rat::GameState original;
  original.set_switch(10, true);
  original.set_variable(3, 99);
  original.set_self_switch("scrap_pile", 'A', true);
  original.add_item("door_key", 1, true);
  original.add_item("potion", 2, false);
  original.set_map_id("grey_yard");
  original.set_player_position(4.5f, 0.0f, -2.0f);

  std::string blob;
  REQUIRE(original.save_to_memory(blob));
  REQUIRE_FALSE(blob.empty());

  rat::GameState restored;
  REQUIRE(restored.load_from_memory(blob));
  REQUIRE(restored.get_switch(10));
  REQUIRE(restored.get_variable(3) == 99);
  REQUIRE(restored.get_self_switch("scrap_pile", 'A'));
  REQUIRE(restored.item_quantity("door_key") == 1);
  REQUIRE(restored.item_quantity("potion") == 2);
  REQUIRE(restored.inventory().at(0).key_item);
  REQUIRE(restored.map_id() == "grey_yard");
  REQUIRE(restored.player_x() == Approx(4.5f));
  REQUIRE(restored.player_y() == Approx(0.0f));
  REQUIRE(restored.player_z() == Approx(-2.0f));
}
