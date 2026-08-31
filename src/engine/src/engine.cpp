#include "rat/engine.hpp"

#include <bgfx/bgfx.h>

#include <cstdio>

namespace rat {

Engine::~Engine() {
  shutdown();
}

bool Engine::init(const RendererConfig& config) {
  if (initialized_) {
    return true;
  }
  if (!renderer_.init(config)) {
    return false;
  }
  if (!greybox_.init()) {
    std::fprintf(stderr, "GreyboxScene::init failed\n");
    renderer_.shutdown();
    return false;
  }
  greybox_.resize(config.width, config.height);
  initialized_ = true;
  return true;
}

void Engine::shutdown() {
  if (!initialized_) {
    return;
  }
  greybox_.shutdown();
  renderer_.shutdown();
  initialized_ = false;
}

void Engine::resize(std::uint32_t width, std::uint32_t height) {
  renderer_.resize(width, height);
  greybox_.resize(width, height);
}

void Engine::begin_frame() {
  if (!initialized_) {
    return;
  }

  renderer_.begin_frame();
  greybox_.draw(0);

  const auto& cam = greybox_.camera();
  bgfx::dbgTextClear();
  bgfx::dbgTextPrintf(1, 1, 0x0f, "%s", debug_banner_.c_str());
  bgfx::dbgTextPrintf(1, 3, 0x0a, "ortho 3/4 greybox  scale=%d", cam.pixel_scale);
}

void Engine::end_frame() {
  if (!initialized_) {
    return;
  }
  renderer_.end_frame();
}

void Engine::frame() {
  begin_frame();
  end_frame();
}

}  // namespace rat
