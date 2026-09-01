#include <rat/buffered_press.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("BufferedPress retains press until consumed", "[unit][input]") {
  rat::BufferedPress press;
  rat::push_buffered_press(press, 0.1f);
  REQUIRE(rat::has_buffered_press(press));

  rat::tick_buffered_press(press, 0.04f);
  REQUIRE(rat::has_buffered_press(press));
  REQUIRE(rat::consume_buffered_press(press));
  REQUIRE_FALSE(rat::has_buffered_press(press));
  REQUIRE_FALSE(rat::consume_buffered_press(press));
}

TEST_CASE("BufferedPress expires after short timeout", "[unit][input]") {
  rat::BufferedPress press;
  rat::push_buffered_press(press, 0.1f);
  rat::tick_buffered_press(press, 0.11f);
  REQUIRE_FALSE(rat::has_buffered_press(press));
  REQUIRE_FALSE(rat::consume_buffered_press(press));
}

TEST_CASE("BufferedPress clear drops pending press", "[unit][input]") {
  rat::BufferedPress press;
  rat::push_buffered_press(press, 0.1f);
  REQUIRE(rat::has_buffered_press(press));

  rat::clear_buffered_press(press);
  REQUIRE_FALSE(rat::has_buffered_press(press));
}

TEST_CASE("BufferedPress consume happens before tick expiry", "[unit][input]") {
  rat::BufferedPress press;
  rat::push_buffered_press(press, 0.1f);

  const bool consumed = rat::consume_then_tick_buffered_press(press, true, 0.1f);
  REQUIRE(consumed);
  REQUIRE_FALSE(rat::has_buffered_press(press));
}

TEST_CASE("BufferedPress expires when not consumed in hitch frame", "[unit][input]") {
  rat::BufferedPress press;
  rat::push_buffered_press(press, 0.1f);

  const bool consumed = rat::consume_then_tick_buffered_press(press, false, 0.1f);
  REQUIRE_FALSE(consumed);
  REQUIRE_FALSE(rat::has_buffered_press(press));
}

TEST_CASE("BufferedPress editor wiring blocks push in edit and capture", "[unit][input]") {
  rat::BufferedPress press;

  rat::push_buffered_press_if_allowed(press, true, false, false, 0.1f);
  REQUIRE_FALSE(rat::has_buffered_press(press));

  rat::push_buffered_press_if_allowed(press, true, true, true, 0.1f);
  REQUIRE_FALSE(rat::has_buffered_press(press));

  rat::push_buffered_press_if_allowed(press, true, true, false, 0.1f);
  REQUIRE(rat::has_buffered_press(press));
}

TEST_CASE("BufferedPress capture clears pending press", "[unit][input]") {
  rat::BufferedPress press;
  rat::push_buffered_press(press, 0.1f);
  REQUIRE(rat::has_buffered_press(press));

  rat::clear_buffered_press_if_captured(press, true);
  REQUIRE_FALSE(rat::has_buffered_press(press));
}
