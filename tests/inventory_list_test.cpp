#include <rat/game_state.hpp>
#include <rat/inventory_list.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("format_inventory_rows shows empty inventory", "[unit][inventory]") {
  REQUIRE(rat::format_inventory_rows({}).size() == 1);
  REQUIRE(rat::format_inventory_rows({})[0] == "(empty)");
}

TEST_CASE("format_inventory_rows lists id quantity and key flag", "[unit][inventory]") {
  rat::GameState state;
  state.add_item("rusty_cog", 1, true);
  state.add_item("scrap", 3, false);

  const auto rows = rat::format_inventory_rows(state.inventory());
  REQUIRE(rows.size() == 2);
  REQUIRE(rows[0] == "rusty_cog  x1  key");
  REQUIRE(rows[1] == "scrap  x3");
}
