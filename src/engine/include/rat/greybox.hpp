#pragma once

#include "rat/camera.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/terrain_geometry.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <span>
#include <vector>

namespace rat {

struct DebugColorVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  std::uint32_t abgr = 0xffffffff;
};

// GPU grey-box helpers: flat floor + grid under switchable ortho camera (view 0).
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
  void set_camera_mode(CameraMode mode);
  void set_player(const PlayerBody& player) {
    player_ = player;
    has_player_ = true;
  }
  void set_blockers(std::span<const BlockerDef> blockers);
  void set_event_markers(std::span<const Vec3> markers);
  void set_terrain_map(const MapData& map);
  void set_selected_blocker(int index);  // -1 = none
  void set_selected_event_marker(int index);  // -1 = none

  void draw(bgfx::ViewId view_id = 0);

  [[nodiscard]] bool is_initialized() const { return initialized_; }
  [[nodiscard]] CameraMode camera_mode() const { return params_.mode; }
  [[nodiscard]] const OrthoCamera& camera() const { return camera_; }
  [[nodiscard]] Vec3 camera_focus() const { return params_.focus; }

 private:
  void rebuild_camera();

  bool initialized_ = false;
  bool has_player_ = false;
  std::uint32_t width_ = 1;
  std::uint32_t height_ = 1;
  OrthoCameraParams params_{};
  OrthoCamera camera_{};
  PlayerBody player_{};
  std::vector<BlockerDef> blockers_;
  std::vector<Vec3> event_markers_;
  TerrainGeometry terrain_geometry_{};
  std::vector<DebugColorVertex> terrain_vertex_data_;
  std::vector<std::uint16_t> terrain_indices_;
  std::vector<DebugColorVertex> terrain_grid_line_data_;
  int selected_blocker_ = -1;
  int selected_event_marker_ = -1;

  bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
  bgfx::VertexLayout layout_;
};

}  // namespace rat
