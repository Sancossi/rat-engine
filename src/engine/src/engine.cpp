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

  // Sample blocker so collision is visible in the editor immediately.
  blockers_ = {BlockerDef{.bounds = Aabb2{3.0f, -1.0f, 5.0f, 1.0f}}};
  player_ = {};
  greybox_.set_player(player_);
  greybox_.set_blockers(blockers_);

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

void Engine::set_player(const PlayerBody& player) {
  player_ = player;
  greybox_.set_player(player_);
}

void Engine::set_blockers(std::vector<BlockerDef> blockers) {
  blockers_ = std::move(blockers);
  greybox_.set_blockers(blockers_);
}

void Engine::set_event_markers(std::vector<Vec3> markers) {
  greybox_.set_event_markers(markers);
}

void Engine::set_terrain_map(const MapData& map) {
  terrain_map_ = map;
  greybox_.set_terrain_map(terrain_map_);
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
  bgfx::dbgTextPrintf(1, 3, 0x0a, "%s  scale=%d  WASD | C camera | F2 mode",
                      camera_mode_name(cam.mode), cam.pixel_scale);
  bgfx::dbgTextPrintf(1, 4, 0x0b, "player (%.2f, %.2f, %.2f)", player_.x, player_.y, player_.z);
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
