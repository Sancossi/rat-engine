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
  const Vec3 f = normalize(sub(at, eye));
  const Vec3 s = normalize(cross(f, up));
  const Vec3 u = cross(s, f);

  Mat4 out{};
  out.m[0] = s.x;
  out.m[4] = s.y;
  out.m[8] = s.z;
  out.m[1] = u.x;
  out.m[5] = u.y;
  out.m[9] = u.z;
  out.m[2] = -f.x;
  out.m[6] = -f.y;
  out.m[10] = -f.z;
  out.m[12] = -dot(s, eye);
  out.m[13] = -dot(u, eye);
  out.m[14] = dot(f, eye);
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

}  // namespace

OrthoCamera build_ortho_three_quarter(std::uint32_t framebuffer_width,
                                      std::uint32_t framebuffer_height,
                                      const OrthoCameraParams& params) {
  OrthoCamera cam;
  const float fb_w = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_width));
  const float fb_h = static_cast<float>(std::max<std::uint32_t>(1, framebuffer_height));
  const float tiles_y = std::max(1.0f, params.visible_tiles_y);
  const int base_px = std::max(1, params.base_pixels_per_tile);

  cam.pixel_scale = std::max(1, static_cast<int>(std::floor(fb_h / (tiles_y * static_cast<float>(base_px)))));
  cam.half_height_world = 0.5f * tiles_y * params.tile_size;
  cam.half_width_world = cam.half_height_world * (fb_w / fb_h);

  // Classic 3/4: elevated and offset on XZ so floor reads as readable "front" diamond.
  constexpr float kOffset = 16.0f;
  cam.eye = {params.focus.x + kOffset, params.focus.y + kOffset * 1.25f, params.focus.z + kOffset};
  cam.view = look_at(cam.eye, params.focus, {0.0f, 1.0f, 0.0f});
  cam.proj = ortho(-cam.half_width_world, cam.half_width_world, -cam.half_height_world,
                   cam.half_height_world, params.near_plane, params.far_plane);
  return cam;
}

}  // namespace rat
