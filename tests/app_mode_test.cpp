#include <rat/app_mode.hpp>

#include <catch2/catch_test_macros.hpp>
#include <cstring>

TEST_CASE("AppMode toggles Play <-> Edit", "[unit][app_mode]") {
  REQUIRE(rat::toggle_app_mode(rat::AppMode::Play) == rat::AppMode::Edit);
  REQUIRE(rat::toggle_app_mode(rat::AppMode::Edit) == rat::AppMode::Play);
}

TEST_CASE("AppMode names are PLAY and EDIT", "[unit][app_mode]") {
  REQUIRE(std::strcmp(rat::app_mode_name(rat::AppMode::Play), "PLAY") == 0);
  REQUIRE(std::strcmp(rat::app_mode_name(rat::AppMode::Edit), "EDIT") == 0);
}

TEST_CASE("Play enables player and event runtime; Edit pauses both", "[unit][app_mode]") {
  REQUIRE(rat::player_control_enabled(rat::AppMode::Play));
  REQUIRE(rat::event_runtime_enabled(rat::AppMode::Play));

  REQUIRE_FALSE(rat::player_control_enabled(rat::AppMode::Edit));
  REQUIRE_FALSE(rat::event_runtime_enabled(rat::AppMode::Edit));
}
