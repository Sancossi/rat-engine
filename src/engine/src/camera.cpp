#include "rat/camera.hpp"

#include <algorithm>
#include <cmath>

namespace rat {
namespace {

Vec3 sub(Vec3 a, Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 normalize(Vec3 v) {
  const float len = std::sqrt(dot(v, v));
  if (len <= 1e-8f) {
    return {0.0f, 1.0f, 0.0f};
  }
  return {v.x / len, v.y / len, v.z / len};
}

Mat4 look_at(Vec3 eye, Vec3 at, Vec3 up) {
  // Match bx::mtxLookAt Left: rows are right/up/view, m[14] = -dot(view, eye).
  const Vec3 view = normalize(sub(at, eye));
  const Vec3 uxv = cross(up, view);
  const Vec3 right = (dot(uxv, uxv) == 0.0f) ? Vec3{-1.0f, 0.0f, 0.0f} : normalize(uxv);
  const Vec3 up_axis = cross(view, right);

  Mat4 out{};
  out.m[0] = right.x;
  out.m[1] = up_axis.x;
  out.m[2] = view.x;
  out.m[4] = right.y;
  out.m[5] = up_axis.y;
  out.m[6] = view.y;
  out.m[8] = right.z;
  out.m[9] = up_axis.z;
  out.m[10] = view.z;
  out.m[12] = -dot(right, eye);
  out.m[13] = -dot(up_axis, eye);
  out.m[14] = -dot(view, eye);
  out.m[15] = 1.0f;
  return out;
}

// OpenGL-style ortho (z in [-1,1]). bgfx may remap via Caps; Engine applies bx::mtxOrtho at draw time
// when homogeneousDepth differs. For unit tests we keep a stable pure-math ortho.
Mat4 ortho(float left, float right, float bottom, float top, float near_z, float far_z) {
  Mat4 out{};
  out.m[0] = 2.0f / (right - left);
  out.m[5] = 2.0f / (top - bottom);
  out.m[10] = -2.0f / (far_z - near_z);
  out.m[12] = -(right + left) / (right - left);
  out.m[13] = -(top + bottom) / (top - bottom);
  out.m[14] = -(far_z + near_z) / (far_z - near_z);
  out.m[15] = 1.0f;
  return out;
}

void fill_common_extent(OrthoCamera& cam, std::uint32_t framebuffer_width,
                        std::uint32_t framebuffer_height, const OrthoCameraParams& params) {
  const float fb_w = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_width));
  const float fb_h = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_height));
  const float tiles_y = std::max(1.0f, params.visible_tiles_y);
  const int base_px = std::max(1, params.base_pixels_per_tile);

  cam.pixel_scale =
      std::max(1, static_cast<int>(std::floor(fb_h / (tiles_y * static_cast<float>(base_px)))));
  cam.half_height_world = 0.5f * tiles_y * params.tile_size;
  cam.half_width_world = cam.half_height_world * (fb_w / fb_h);
  cam.proj = ortho(-cam.half_width_world, cam.half_width_world, -cam.half_height_world,
                   cam.half_height_world, params.near_plane, params.far_plane);
}

}  // namespace

CameraMode next_camera_mode(CameraMode mode) {
  switch (mode) {
    case CameraMode::TopDown:
      return CameraMode::Tilt45;
    case CameraMode::Tilt45:
      return CameraMode::ThreeQuarter;
    case CameraMode::ThreeQuarter:
      return CameraMode::TopDown;
  }
  return CameraMode::TopDown;
}

const char* camera_mode_name(CameraMode mode) {
  switch (mode) {
    case CameraMode::TopDown:
      return "ortho top-down";
    case CameraMode::Tilt45:
      return "ortho tilt 45";
    case CameraMode::ThreeQuarter:
      return "ortho 3/4";
  }
  return "ortho";
}

OrthoCamera build_ortho_three_quarter(std::uint32_t framebuffer_width,
                                      std::uint32_t framebuffer_height,
                                      const OrthoCameraParams& params) {
  OrthoCamera cam;
  cam.mode = CameraMode::ThreeQuarter;
  fill_common_extent(cam, framebuffer_width, framebuffer_height, params);

  // Classic 3/4: elevated and offset on XZ so floor reads as readable "front" diamond.
  constexpr float kOffset = 16.0f;
  cam.eye = {params.focus.x + kOffset, params.focus.y + kOffset * 1.25f, params.focus.z + kOffset};
  cam.view = look_at(cam.eye, params.focus, {0.0f, 1.0f, 0.0f});
  return cam;
}

OrthoCamera build_ortho_top_down(std::uint32_t framebuffer_width,
                                 std::uint32_t framebuffer_height,
                                 const OrthoCameraParams& params) {
  OrthoCamera cam;
  cam.mode = CameraMode::TopDown;
  fill_common_extent(cam, framebuffer_width, framebuffer_height, params);

  // Straight above focus. Up = -Z so screen-up matches world -Z (W moves "north").
  constexpr float kHeight = 32.0f;
  cam.eye = {params.focus.x, params.focus.y + kHeight, params.focus.z};
  cam.view = look_at(cam.eye, params.focus, {0.0f, 0.0f, -1.0f});
  return cam;
}

OrthoCamera build_ortho_tilt45(std::uint32_t framebuffer_width,
                               std::uint32_t framebuffer_height,
                               const OrthoCameraParams& params) {
  OrthoCamera cam;
  cam.mode = CameraMode::Tilt45;
  fill_common_extent(cam, framebuffer_width, framebuffer_height, params);

  // Equal height and +Z offset => pitch atan(1)=45°, looking toward -Z (screen up ≈ -Z).
  constexpr float kDist = 24.0f;
  cam.eye = {params.focus.x, params.focus.y + kDist, params.focus.z + kDist};
  cam.view = look_at(cam.eye, params.focus, {0.0f, 1.0f, 0.0f});
  return cam;
}

OrthoCamera build_ortho_camera(std::uint32_t framebuffer_width, std::uint32_t framebuffer_height,
                               const OrthoCameraParams& params) {
  switch (params.mode) {
    case CameraMode::TopDown:
      return build_ortho_top_down(framebuffer_width, framebuffer_height, params);
    case CameraMode::Tilt45:
      return build_ortho_tilt45(framebuffer_width, framebuffer_height, params);
    case CameraMode::ThreeQuarter:
      return build_ortho_three_quarter(framebuffer_width, framebuffer_height, params);
  }
  return build_ortho_top_down(framebuffer_width, framebuffer_height, params);
}

}  // namespace rat
