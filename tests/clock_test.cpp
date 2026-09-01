#include <rat/clock.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("FakeClock reports the seconds it was set to", "[unit][clock]") {
  rat::FakeClock clock;
  CHECK(clock.now_seconds() == Catch::Approx(0.0));
  clock.set_seconds(1.25);
  CHECK(clock.now_seconds() == Catch::Approx(1.25));
}

TEST_CASE("FakeClock advance_seconds moves now forward", "[unit][clock]") {
  rat::FakeClock clock;
  clock.set_seconds(2.0);
  clock.advance_seconds(0.5);
  CHECK(clock.now_seconds() == Catch::Approx(2.5));
}

TEST_CASE("SteadyClock now_seconds is monotonic", "[unit][clock]") {
  rat::SteadyClock clock;
  const double first = clock.now_seconds();
  const double second = clock.now_seconds();
  CHECK(second >= first);
}
