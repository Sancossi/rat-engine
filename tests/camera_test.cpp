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

  REQUIRE(cam.mode == rat::CameraMode::ThreeQuarter);
  REQUIRE(cam.eye.y > params.focus.y);
  REQUIRE(cam.eye.z > params.focus.z);
  REQUIRE(std::abs(cam.eye.x - params.focus.x) > 0.1f);

  REQUIRE(cam.proj.m[15] == Approx(1.0f));
  REQUIRE(cam.proj.m[11] == Approx(0.0f).margin(0.0001f));
}

TEST_CASE("Ortho top-down looks straight down at focus", "[unit][camera]") {
  rat::OrthoCameraParams params;
  params.focus = {3.0f, 0.0f, -1.0f};
  params.mode = rat::CameraMode::TopDown;
  const auto cam = rat::build_ortho_top_down(640, 480, params);

  REQUIRE(cam.mode == rat::CameraMode::TopDown);
  REQUIRE(cam.eye.x == Approx(params.focus.x));
  REQUIRE(cam.eye.z == Approx(params.focus.z));
  REQUIRE(cam.eye.y > params.focus.y);

  const auto via_dispatch = rat::build_ortho_camera(640, 480, params);
  REQUIRE(via_dispatch.mode == rat::CameraMode::TopDown);
  REQUIRE(via_dispatch.eye.y == Approx(cam.eye.y));
}

TEST_CASE("Ortho tilt 45 looks from elevated +Z with equal height", "[unit][camera]") {
  rat::OrthoCameraParams params;
  params.focus = {0.0f, 0.0f, 0.0f};
  params.mode = rat::CameraMode::Tilt45;
  const auto cam = rat::build_ortho_tilt45(640, 480, params);

  REQUIRE(cam.mode == rat::CameraMode::Tilt45);
  REQUIRE(cam.eye.x == Approx(params.focus.x));
  REQUIRE(cam.eye.y - params.focus.y == Approx(cam.eye.z - params.focus.z));
  REQUIRE(cam.eye.y > params.focus.y);
  REQUIRE(cam.eye.z > params.focus.z);

  REQUIRE(rat::next_camera_mode(rat::CameraMode::TopDown) == rat::CameraMode::Tilt45);
  REQUIRE(rat::next_camera_mode(rat::CameraMode::Tilt45) == rat::CameraMode::ThreeQuarter);
  REQUIRE(rat::next_camera_mode(rat::CameraMode::ThreeQuarter) == rat::CameraMode::TopDown);
}
