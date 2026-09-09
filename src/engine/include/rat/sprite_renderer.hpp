#pragma once
#include "rat/camera.hpp"
#include <bgfx/bgfx.h>
#include <filesystem>
#include <string>
namespace rat {
// Owns the prototype atlas and shader; destroy before Renderer::shutdown.
class SpriteRenderer {
 public:
  SpriteRenderer() = default;
  ~SpriteRenderer() { shutdown(); }
  SpriteRenderer(const SpriteRenderer&) = delete;
  SpriteRenderer& operator=(const SpriteRenderer&) = delete;
  bool init(const std::filesystem::path& png, std::string& error);
  void shutdown();
  void draw(const OrthoCamera& camera, Vec3 feet, int direction, int frame,
            bgfx::ViewId view = 0, std::uint32_t tint = 0xffffffff, float height = 1.6f);
 private:
  bgfx::TextureHandle texture_ = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle sampler_ = BGFX_INVALID_HANDLE;
  bgfx::VertexLayout layout_;
};
}
