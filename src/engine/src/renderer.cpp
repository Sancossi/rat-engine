#include "rat/renderer.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

namespace rat {

Renderer::~Renderer() {
  shutdown();
}

bool Renderer::init(const RendererConfig& config) {
  if (initialized_) {
    return true;
  }
  if (config.window.nwh == nullptr) {
    return false;
  }

  width_ = config.width < 1 ? 1 : config.width;
  height_ = config.height < 1 ? 1 : config.height;
  reset_flags_ = config.vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;

  // Single-threaded mode: required when driving bgfx from the Qt UI thread.
  bgfx::renderFrame();

  bgfx::Init init;
  init.type = bgfx::RendererType::Count;  // auto-select (D3D11 preferred on Win)
#if BX_PLATFORM_WINDOWS
  init.type = bgfx::RendererType::Direct3D11;
#endif
  init.platformData.nwh = config.window.nwh;
  init.platformData.ndt = config.window.ndt;
  init.resolution.width = width_;
  init.resolution.height = height_;
  init.resolution.reset = reset_flags_;

  if (!bgfx::init(init)) {
    return false;
  }

  bgfx::setDebug(BGFX_DEBUG_TEXT);
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1a1a2eff, 1.0f, 0);
  bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width_), static_cast<uint16_t>(height_));

  initialized_ = true;
  return true;
}

void Renderer::shutdown() {
  if (!initialized_) {
    return;
  }
  bgfx::shutdown();
  initialized_ = false;
}

void Renderer::resize(std::uint32_t width, std::uint32_t height) {
  if (!initialized_) {
    return;
  }
  width_ = width < 1 ? 1 : width;
  height_ = height < 1 ? 1 : height;
  bgfx::reset(width_, height_, reset_flags_);
  bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width_), static_cast<uint16_t>(height_));
}

void Renderer::begin_frame() {
  if (!initialized_) {
    return;
  }
  bgfx::touch(0);
}

void Renderer::end_frame() {
  if (!initialized_) {
    return;
  }
  bgfx::frame();
}

}  // namespace rat
