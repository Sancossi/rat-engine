#include <rat/camera.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using Catch::Approx;

TEST_CASE("Ortho camera uses integer pixel scale from 32px tiles", "[unit][camera]") {
  rat::OrthoCameraParams params;
  params.base_pixels_per_tile = 32;
  params.visible_tiles_y = 10.0f;

  // 640 tall -> scale 2 (640 / (10*32) = 2)
  const auto cam = rat::build_ortho_three_quarter(800, 640, params);
  REQUIRE(cam.pixel_scale == 2);
  REQUIRE(cam.half_height_world == Approx(5.0f));  // 10 tiles / 2
  REQUIRE(cam.half_width_world == Approx(5.0f * (800.0f / 640.0f)));
}

TEST_CASE("Ortho camera clamps scale to at least 1", "[unit][camera]") {
  rat::OrthoCameraParams params;
  params.base_pixels_per_tile = 32;
  params.visible_tiles_y = 20.0f;
  const auto cam = rat::build_ortho_three_quarter(320, 200, params);
  REQUIRE(cam.pixel_scale == 1);
}

TEST_CASE("Ortho 3/4 view looks from elevated forward offset", "[unit][camera]") {
  rat::OrthoCameraParams params;
  params.focus = {1.0f, 0.0f, 2.0f};
  const auto cam = rat::build_ortho_three_quarter(640, 480, params);

  // View matrix maps world eye toward -Z in view space; translation column encodes -eye*R.
  // Spot-check: eye is above and behind focus on XZ diagonal (classic 3/4).
  REQUIRE(cam.eye.y > params.focus.y);
  REQUIRE(cam.eye.z > params.focus.z);
  REQUIRE(std::abs(cam.eye.x - params.focus.x) > 0.1f);

  // Projection is orthographic: m[11] is typically -1 or 0 depending on depth range convention;
  // perspective would have m[11] == -1 and m[14] != 0 with m[15]==0. Our ortho keeps m[15]==1.
  REQUIRE(cam.proj.m[15] == Approx(1.0f));
  REQUIRE(cam.proj.m[11] == Approx(0.0f).margin(0.0001f));
}
