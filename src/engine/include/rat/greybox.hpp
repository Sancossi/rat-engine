#pragma once

#include "rat/camera.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>

namespace rat {

// GPU grey-box helpers: flat floor + grid under fixed ortho 3/4 camera (view 0).
class GreyboxScene {
 public:
  GreyboxScene() = default;
  ~GreyboxScene();

  GreyboxScene(const GreyboxScene&) = delete;
  GreyboxScene& operator=(const GreyboxScene&) = delete;

  bool init();
  void shutdown();
  void resize(std::uint32_t width, std::uint32_t height);
  void set_focus(float x, float y, float z);
  void draw(bgfx::ViewId view_id = 0);

  [[nodiscard]] bool is_initialized() const { return initialized_; }
  [[nodiscard]] const OrthoCamera& camera() const { return camera_; }

 private:
  void rebuild_camera();

  bool initialized_ = false;
  std::uint32_t width_ = 1;
  std::uint32_t height_ = 1;
  OrthoCameraParams params_{};
  OrthoCamera camera_{};

  bgfx::ProgramHandle program_lines_ = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle program_fill_ = BGFX_INVALID_HANDLE;
  bgfx::VertexLayout layout_lines_;
  bgfx::VertexLayout layout_fill_;
};

}  // namespace rat
