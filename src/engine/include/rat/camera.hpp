#pragma once

#include <cstdint>

namespace rat {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct Mat4 {
  float m[16] = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
  };
};

struct OrthoCameraParams {
  Vec3 focus{0.0f, 0.0f, 0.0f};
  float tile_size = 1.0f;
  int base_pixels_per_tile = 32;
  float visible_tiles_y = 12.0f;
  float near_plane = 0.1f;
  float far_plane = 500.0f;
};

struct OrthoCamera {
  Mat4 view;
  Mat4 proj;
  Vec3 eye;
  int pixel_scale = 1;
  float half_width_world = 1.0f;
  float half_height_world = 1.0f;
};

[[nodiscard]] OrthoCamera build_ortho_three_quarter(std::uint32_t framebuffer_width,
                                                    std::uint32_t framebuffer_height,
                                                    const OrthoCameraParams& params = {});

}  // namespace rat
