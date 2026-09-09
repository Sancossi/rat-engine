#pragma once

#include "rat/camera.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/terrain_geometry.hpp"
#include "rat/surface_visual.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <optional>
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
  void set_camera_override(std::optional<ClimbCameraPose> pose);
  void set_player_visible(bool visible) { player_visible_ = visible; }
  void set_player(const PlayerBody& player);
  void set_climb_lock(bool locked, float into_x, float into_z);
  void tick(float dt);
  void set_blockers(std::span<const BlockerDef> blockers);
  void set_event_markers(std::span<const Vec3> markers);
  void set_terrain_map(const MapData& map);
  void set_selected_blocker(int index);  // -1 = none
  void set_selected_event_marker(int index);  // -1 = none

  void draw(bgfx::ViewId view_id = 0);

  [[nodiscard]] bool is_initialized() const { return initialized_; }
  [[nodiscard]] CameraMode camera_mode() const { return params_.mode; }
  [[nodiscard]] const OrthoCamera& camera() const { return camera_; }
  [[nodiscard]] Vec3 camera_focus() const {
    return has_display_ ? display_pose_.focus : params_.focus;
  }

 private:
  void begin_camera_turn();
  void rebuild_camera();
  void rebuild_terrain_visuals();

  bool initialized_ = false;
  bool has_player_ = false;
  bool player_visible_ = true;
  bool climb_locked_ = false;
  bool has_display_ = false;
  float climb_into_x_ = 0.0f;
  float climb_into_z_ = 0.0f;
  float turn_t_ = 1.0f;
  ClimbCameraPose from_pose_{};
  ClimbCameraPose display_pose_{};
  Vec3 from_up_{0.0f, 1.0f, 0.0f};
  Vec3 display_up_{0.0f, 1.0f, 0.0f};
  std::uint32_t width_ = 1;
  std::uint32_t height_ = 1;
  OrthoCameraParams params_{};
  OrthoCamera camera_{};
  std::optional<ClimbCameraPose> camera_override_;
  PlayerBody player_{};
  MapData terrain_map_{};
  bool has_terrain_map_ = false;
  std::vector<BlockerDef> blockers_;
  std::vector<Vec3> event_markers_;
  TerrainGeometry terrain_geometry_{};
  SurfaceVisualMesh surface_;
  std::vector<DebugColorVertex> terrain_grid_line_data_;
  int selected_blocker_ = -1;
  int selected_event_marker_ = -1;

  bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle surface_program_ = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle contour_program_ = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle surface_uniform_ = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle contour_uniform_ = BGFX_INVALID_HANDLE;
  bgfx::VertexLayout layout_;
  bgfx::VertexLayout surface_layout_;
  bgfx::VertexLayout contour_layout_;
};

}  // namespace rat
