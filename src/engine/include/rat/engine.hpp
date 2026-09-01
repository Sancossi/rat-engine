#pragma once

#include "rat/greybox.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/renderer.hpp"

#include <string>
#include <vector>

namespace rat {

struct RenderWorld;

class Engine {
 public:
  Engine() = default;
  ~Engine();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  bool init(const RendererConfig& config);
  void shutdown();
  void resize(std::uint32_t width, std::uint32_t height);

  // begin_frame: clear + greybox + dbgText. end_frame: bgfx::frame().
  // Call ImGui (or other views) between begin_frame and end_frame.
  void begin_frame();
  void submit_world(const RenderWorld& world);
  void end_frame();
  void frame();

  void set_player(const PlayerBody& player);
  void set_blockers(std::vector<BlockerDef> blockers);
  void set_event_markers(std::vector<Vec3> markers);
  void set_terrain_map(const MapData& map);
  [[nodiscard]] const PlayerBody& player() const { return player_; }
  [[nodiscard]] const std::vector<BlockerDef>& blockers() const { return blockers_; }

  [[nodiscard]] bool is_initialized() const { return initialized_; }
  [[nodiscard]] GreyboxScene& greybox() { return greybox_; }
  [[nodiscard]] const GreyboxScene& greybox() const { return greybox_; }

  void set_debug_banner(std::string banner) { debug_banner_ = std::move(banner); }

 private:
  Renderer renderer_;
  GreyboxScene greybox_;
  PlayerBody player_{};
  std::vector<BlockerDef> blockers_;
  MapData terrain_map_{};
  bool initialized_ = false;
  std::string debug_banner_{"rat-engine"};
};

}  // namespace rat
