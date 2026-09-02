#include <rat/camera.hpp>
#include <rat/player.hpp>

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

TEST_CASE("Climb camera sits behind the player looking into an east face", "[unit][camera]") {
  const rat::Vec3 player{0.85f, 1.0f, 0.5f};
  const rat::ClimbCameraPose pose = rat::climb_camera_pose(player, 1.0f, 0.0f);
  REQUIRE(pose.focus.x == Approx(player.x));
  REQUIRE(pose.focus.y == Approx(player.y));
  REQUIRE(pose.focus.z == Approx(player.z));
  REQUIRE(pose.eye.x < player.x - 1.0f);
  REQUIRE(pose.eye.y > player.y);
  const rat::MoveInput w =
      rat::camera_relative_move(0.0f, 1.0f, pose.eye, pose.focus);
  REQUIRE(w.axis_x > 0.5f);
}

TEST_CASE("Lerp climb camera pose interpolates eye and focus", "[unit][camera]") {
  const rat::ClimbCameraPose from{{0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f}};
  const rat::ClimbCameraPose to{{10.0f, 20.0f, 30.0f}, {5.0f, 8.0f, 11.0f}};

  const rat::ClimbCameraPose at_from = rat::lerp_climb_camera_pose(from, to, 0.0f);
  REQUIRE(at_from.eye.x == Approx(from.eye.x));
  REQUIRE(at_from.eye.y == Approx(from.eye.y));
  REQUIRE(at_from.eye.z == Approx(from.eye.z));
  REQUIRE(at_from.focus.x == Approx(from.focus.x));
  REQUIRE(at_from.focus.y == Approx(from.focus.y));
  REQUIRE(at_from.focus.z == Approx(from.focus.z));

  const rat::ClimbCameraPose at_to = rat::lerp_climb_camera_pose(from, to, 1.0f);
  REQUIRE(at_to.eye.x == Approx(to.eye.x));
  REQUIRE(at_to.eye.y == Approx(to.eye.y));
  REQUIRE(at_to.eye.z == Approx(to.eye.z));
  REQUIRE(at_to.focus.x == Approx(to.focus.x));
  REQUIRE(at_to.focus.y == Approx(to.focus.y));
  REQUIRE(at_to.focus.z == Approx(to.focus.z));

  const rat::ClimbCameraPose mid = rat::lerp_climb_camera_pose(from, to, 0.5f);
  REQUIRE(mid.eye.x == Approx(5.0f));
  REQUIRE(mid.eye.y == Approx(10.0f));
  REQUIRE(mid.eye.z == Approx(15.0f));
  REQUIRE(mid.focus.x == Approx(3.0f));
  REQUIRE(mid.focus.y == Approx(5.0f));
  REQUIRE(mid.focus.z == Approx(7.0f));

  const rat::ClimbCameraPose clamped_lo = rat::lerp_climb_camera_pose(from, to, -1.0f);
  REQUIRE(clamped_lo.eye.x == Approx(from.eye.x));
  const rat::ClimbCameraPose clamped_hi = rat::lerp_climb_camera_pose(from, to, 2.0f);
  REQUIRE(clamped_hi.eye.x == Approx(to.eye.x));
}
