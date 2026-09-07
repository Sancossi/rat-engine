#include <rat/file_store.hpp>
#include <rat/game_state.hpp>
#include <rat/save_game.hpp>
#include <nlohmann/json.hpp>
#include <limits>

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

TEST_CASE("save_game round-trips GameState through MemoryFileStore", "[unit][gamestate][file]") {
  rat::GameState original;
  original.set_switch(10, true);
  original.set_variable(3, 99);
  original.add_item("door_key", 1, true);
  original.set_map_id("grey_yard");
  original.set_player_position(4.5f, 1.25f, -2.0f);

  rat::MemoryFileStore files;
  const rat::GameFileResult saved = rat::save_game(files, "saves/slot1.ratsave", original);
  REQUIRE(saved.ok);

  rat::GameState restored;
  const rat::GameFileResult loaded = rat::load_game(files, "saves/slot1.ratsave", restored);
  REQUIRE(loaded.ok);
  REQUIRE(restored.get_switch(10));
  REQUIRE(restored.get_variable(3) == 99);
  REQUIRE(restored.item_quantity("door_key") == 1);
  REQUIRE(restored.map_id() == "grey_yard");
  REQUIRE(restored.player_x() == Approx(4.5f));
  REQUIRE(restored.player_y() == Approx(1.25f));
  REQUIRE(restored.player_z() == Approx(-2.0f));
}

TEST_CASE("load_game fails on missing path and leaves GameState unchanged",
          "[unit][gamestate][file]") {
  rat::GameState state;
  state.set_switch(1, true);
  state.set_map_id("keep");
  state.set_player_position(1.0f, 2.0f, 3.0f);

  rat::MemoryFileStore files;
  const rat::GameFileResult loaded = rat::load_game(files, "saves/missing.ratsave", state);
  REQUIRE_FALSE(loaded.ok);
  REQUIRE_FALSE(loaded.error.empty());
  REQUIRE(state.get_switch(1));
  REQUIRE(state.map_id() == "keep");
  REQUIRE(state.player_x() == Approx(1.0f));
}

TEST_CASE("load_game fails on garbage bytes and leaves GameState unchanged",
          "[unit][gamestate][file]") {
  rat::GameState state;
  state.set_switch(1, true);
  state.set_variable(2, 7);
  state.add_item("coin", 3);
  state.set_map_id("keep");

  rat::MemoryFileStore files;
  REQUIRE(files.write("saves/slot1.ratsave", "not a ratsave").ok);

  const rat::GameFileResult loaded = rat::load_game(files, "saves/slot1.ratsave", state);
  REQUIRE_FALSE(loaded.ok);
  REQUIRE_FALSE(loaded.error.empty());
  REQUIRE(state.get_switch(1));
  REQUIRE(state.get_variable(2) == 7);
  REQUIRE(state.item_quantity("coin") == 3);
  REQUIRE(state.map_id() == "keep");
}

TEST_CASE("Save v2 preserves Unicode spaces order zero quantities and float precision", "[unit][gamestate][storage]") {
  rat::GameState original;
  original.set_map_id(reinterpret_cast<const char*>(u8"yard with spaces \u0434\u0432\u043e\u0440"));
  original.set_self_switch(reinterpret_cast<const char*>(u8"NPC with spaces \u043a\u0440\u044b\u0441\u0430"), 'D', true);
  original.add_item("first item", 1, true);
  original.add_item("second item", 7);
  original.add_item("first item", -1);
  original.set_player_position(1.2345678f, -1234.5678f, 0.000000123f);
  original.set_switch(0xffffffffu, false);
  original.set_variable(2, (std::numeric_limits<int>::min)());
  std::string blob;
  REQUIRE(original.save_to_memory(blob));
  REQUIRE(nlohmann::json::parse(blob).at("schema_version") == 2);
  rat::GameState restored;
  REQUIRE(restored.load_from_memory(blob));
  CHECK(restored.map_id() == original.map_id());
  CHECK(restored.player_x() == original.player_x());
  CHECK(restored.player_y() == original.player_y());
  CHECK(restored.player_z() == original.player_z());
  CHECK(restored.get_self_switch(reinterpret_cast<const char*>(u8"NPC with spaces \u043a\u0440\u044b\u0441\u0430"), 'D'));
  REQUIRE(restored.inventory().size() == 2);
  CHECK(restored.inventory()[0].id == "first item");
  CHECK(restored.inventory()[0].quantity == 0);
  CHECK(restored.inventory()[1].id == "second item");
  CHECK(restored.get_variable(2) == (std::numeric_limits<int>::min)());
}

TEST_CASE("Strict legacy reader accepts valid progress and rejects damaged records transactionally", "[unit][gamestate][storage]") {
  const std::string header = "RATSAVE1\nmap yard\npos 1 2 3\n";
  rat::GameState state;
  REQUIRE(state.load_from_memory(header + "sw 1 1\nvar 2 -5\nss npc 8\nitem key 1 1\n"));
  CHECK(state.get_switch(1));
  CHECK(state.get_variable(2) == -5);
  CHECK(state.get_self_switch("npc", 'D'));
  std::string before;
  REQUIRE(state.save_to_memory(before));
  for (const auto& bad : {std::string("RATSAVE1\n"), std::string("RATSAVE1\nmap yard\npos NaN 0 0\n"),
       header + "sw 1 2\n", header + "sw 1 0 trailing\n", header + "var 1 2147483648\n",
       header + "pos 0 0 0\n", header + "map another\n", header + "ss npc 16\n",
       header + "sw 1 0\nsw 1 1\n", header + "item key 1 0\nitem key 2 0\n",
       std::string("RATSAVE1\nmap \npos 0 0 0\n"), header + "item key -1 0\n"}) {
    INFO(bad);
    CHECK_FALSE(state.load_from_memory(bad));
    std::string after;
    REQUIRE(state.save_to_memory(after));
    CHECK(after == before);
  }
}

TEST_CASE("Save JSON validates required fields duplicates types versions and numeric ranges", "[unit][gamestate][storage]") {
  rat::GameState state;
  state.set_map_id("keep");
  state.set_variable(7, 42);
  std::string before;
  REQUIRE(state.save_to_memory(before));
  const auto valid = nlohmann::json::parse(before);
  auto reject = [&](const std::string& blob) {
    INFO(blob);
    CHECK_FALSE(state.load_from_memory(blob));
    std::string after;
    REQUIRE(state.save_to_memory(after));
    CHECK(after == before);
  };
  for (const char* field : {"schema_version", "map_id", "position", "switches", "variables", "self_switches", "inventory"}) {
    auto bad = valid;
    bad.erase(field);
    reject(bad.dump());
  }
  for (const auto& value : {nlohmann::json(3), nlohmann::json(2.0), nlohmann::json("2")}) {
    auto bad = valid; bad["schema_version"] = value; reject(bad.dump());
  }
  auto bad = valid; bad["position"]["x"] = 1e100; reject(bad.dump());
  bad = valid; bad["switches"] = {{{"id", -1}, {"value", true}}}; reject(bad.dump());
  bad = valid; bad["variables"] = {{{"id", 1}, {"value", 1.5}}}; reject(bad.dump());
  bad = valid; bad["variables"].push_back(bad["variables"][0]); reject(bad.dump());
  bad = valid; bad["switches"] = {{{"id", 1}, {"value", 1}}}; reject(bad.dump());
  bad = valid; bad["self_switches"] = {{{"event_id", "npc"}, {"bits", 256}}}; reject(bad.dump());
  bad = valid; bad["inventory"] = {{{"id", "key"}, {"quantity", 1}, {"key_item", false}}, {{"id", "key"}, {"quantity", 2}, {"key_item", false}}}; reject(bad.dump());
  reject("{\"map_id\":\"duplicate\"," + before.substr(1));
  reject(before.substr(0, before.size() - 1));
}

TEST_CASE("Invalid game state cannot replace an existing save", "[unit][gamestate][storage]") {
  rat::MemoryFileStore files;
  REQUIRE(files.write("slot", "previous bytes").ok);
  rat::GameState state;
  CHECK_FALSE(rat::save_game(files, "slot", state).ok);
  state.set_map_id("yard");
  state.set_player_position(std::numeric_limits<float>::infinity(), 0, 0);
  CHECK_FALSE(rat::save_game(files, "slot", state).ok);
  CHECK(files.read("slot").bytes.as_text() == "previous bytes");
}
