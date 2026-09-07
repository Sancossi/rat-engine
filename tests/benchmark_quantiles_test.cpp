#include "../benchmarks/quantiles.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Benchmark reports nearest-rank quantiles without interpolation", "[benchmark]") {
  using rat::benchmark::quantile;
  REQUIRE(quantile({5,1,3,2,4}, .5) == 3);
  REQUIRE(quantile({5,1,3,2,4}, .95) == 5);
  std::vector<double> hundred;
  for (int i=100; i>0; --i) hundred.push_back(i);
  REQUIRE(quantile(hundred, .5) == 50);
  REQUIRE(quantile(hundred, .95) == 95);
  REQUIRE(quantile(hundred, .99) == 99);
  REQUIRE_THROWS(quantile({}, .5));
  REQUIRE_THROWS(quantile({1}, 0));
}
