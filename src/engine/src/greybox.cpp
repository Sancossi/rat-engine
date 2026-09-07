#include "rat/greybox.hpp"

#include "rat/indoor_volume.hpp"

#include <bgfx/embedded_shader.h>
#include <bx/bx.h>
#include <bx/math.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "fs_debugdraw_lines.bin.h"
#include "vs_debugdraw_lines.bin.h"
#include "vs_surface_glsl.bin.h"
#include "fs_surface_glsl.bin.h"
#include "vs_surface_spirv.bin.h"
#include "fs_surface_spirv.bin.h"
#include "vs_contour_glsl.bin.h"
#include "fs_contour_glsl.bin.h"
#include "vs_contour_spirv.bin.h"
#include "fs_contour_spirv.bin.h"
#if defined(_WIN32)
#include "vs_surface_dxbc.bin.h"
#include "fs_surface_dxbc.bin.h"
#include "vs_contour_dxbc.bin.h"
#include "fs_contour_dxbc.bin.h"
#endif

namespace rat {
namespace {
struct ContourVertex { float x,y,z,other_x,other_y,other_z,side,padding; };

bgfx::ProgramHandle surface_program(bool contour) {
  const auto make=[](const auto& vs,const auto& fs) {
    return bgfx::createProgram(bgfx::createShader(bgfx::copy(vs,sizeof(vs))),
                               bgfx::createShader(bgfx::copy(fs,sizeof(fs))),true);
  };
  switch(bgfx::getRendererType()) {
    case bgfx::RendererType::OpenGL:
      return contour ? make(vs_contour_glsl,fs_contour_glsl) : make(vs_surface_glsl,fs_surface_glsl);
    case bgfx::RendererType::Vulkan:
      return contour ? make(vs_contour_spirv,fs_contour_spirv) : make(vs_surface_spirv,fs_surface_spirv);
#if defined(_WIN32)
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12:
      return contour ? make(vs_contour_dxbc,fs_contour_dxbc) : make(vs_surface_dxbc,fs_surface_dxbc);
#endif
    default: return BGFX_INVALID_HANDLE;
  }
}

// Position + Color0 only — matches vs/fs_debugdraw_lines (u_modelViewProj + vertex color).
// Do NOT use vs_debugdraw_fill here: that shader expects a_indices and u_matColor.
const bgfx::EmbeddedShader k_shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_debugdraw_lines),
    BGFX_EMBEDDED_SHADER(fs_debugdraw_lines),
    BGFX_EMBEDDED_SHADER_END()};

void destroy_program(bgfx::ProgramHandle& handle) {
  if (bgfx::isValid(handle)) {
    bgfx::destroy(handle);
    handle = BGFX_INVALID_HANDLE;
  }
}

}  // namespace

GreyboxScene::~GreyboxScene() {
  shutdown();
}

bool GreyboxScene::init() {
  if (initialized_) {
    return true;
  }

  layout_.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();
  surface_layout_.begin().add(bgfx::Attrib::Position,3,bgfx::AttribType::Float)
      .add(bgfx::Attrib::Normal,3,bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0,4,bgfx::AttribType::Uint8,true).end();
  contour_layout_.begin().add(bgfx::Attrib::Position,3,bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0,3,bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord1,2,bgfx::AttribType::Float).end();
  surface_program_=surface_program(false);
  contour_program_=surface_program(true);
  surface_uniform_=bgfx::createUniform("u_surface",bgfx::UniformType::Vec4);
  contour_uniform_=bgfx::createUniform("u_contour",bgfx::UniformType::Vec4);

  const bgfx::RendererType::Enum type = bgfx::getRendererType();
  program_ = bgfx::createProgram(
      bgfx::createEmbeddedShader(k_shaders, type, "vs_debugdraw_lines"),
      bgfx::createEmbeddedShader(k_shaders, type, "fs_debugdraw_lines"), true);

  if (!bgfx::isValid(program_) || !bgfx::isValid(surface_program_) || !bgfx::isValid(contour_program_)) {
    shutdown();
    return false;
  }

  rebuild_camera();
  initialized_ = true;
  return true;
}

void GreyboxScene::shutdown() {
  destroy_program(program_);
  destroy_program(surface_program_);
  destroy_program(contour_program_);
  if(bgfx::isValid(surface_uniform_)) { bgfx::destroy(surface_uniform_);surface_uniform_=BGFX_INVALID_HANDLE; }
  if(bgfx::isValid(contour_uniform_)) { bgfx::destroy(contour_uniform_);contour_uniform_=BGFX_INVALID_HANDLE; }
  initialized_ = false;
}

void GreyboxScene::resize(std::uint32_t width, std::uint32_t height) {
  width_ = width < 1 ? 1 : width;
  height_ = height < 1 ? 1 : height;
  rebuild_camera();
}

void GreyboxScene::set_focus(float x, float y, float z) {
  params_.focus = {x, y, z};
  rebuild_camera();
}

void GreyboxScene::set_camera_mode(CameraMode mode) {
  if (params_.mode == mode) {
    return;
  }
  begin_camera_turn();
  params_.mode = mode;
  rebuild_camera();
}

void GreyboxScene::set_climb_lock(bool locked, float into_x, float into_z) {
  const bool changed =
      climb_locked_ != locked || (locked && (climb_into_x_ != into_x || climb_into_z_ != into_z));
  climb_locked_ = locked;
  climb_into_x_ = into_x;
  climb_into_z_ = into_z;
  if (changed) {
    begin_camera_turn();
  }
  rebuild_camera();
}

void GreyboxScene::tick(float dt) {
  const float step = std::max(0.0f, dt) / kCameraTurnSeconds;
  turn_t_ = std::clamp(turn_t_ + step, 0.0f, 1.0f);
  rebuild_camera();
}

void GreyboxScene::begin_camera_turn() {
  if (!has_display_) {
    return;
  }
  from_pose_ = display_pose_;
  from_up_ = display_up_;
  turn_t_ = 0.0f;
}

void GreyboxScene::set_player(const PlayerBody& player) {
  const bool was_indoor =
      has_player_ && has_terrain_map_ && player_inside_indoor_volume(terrain_map_, player_);
  player_ = player;
  has_player_ = true;
  if (has_terrain_map_) {
    const bool now_indoor = player_inside_indoor_volume(terrain_map_, player_);
    if (was_indoor != now_indoor) {
      rebuild_terrain_visuals();
    }
  }
}

void GreyboxScene::set_blockers(std::span<const BlockerDef> blockers) {
  blockers_.assign(blockers.begin(), blockers.end());
}

void GreyboxScene::set_event_markers(std::span<const Vec3> markers) {
  event_markers_.assign(markers.begin(), markers.end());
}

void GreyboxScene::set_terrain_map(const MapData& map) {
  terrain_map_ = map;
  has_terrain_map_ = true;
  rebuild_terrain_visuals();
}

void GreyboxScene::rebuild_terrain_visuals() {
  surface_ = {};
  terrain_grid_line_data_.clear();
  terrain_geometry_ = {};

  if (!has_terrain_map_ ||
      choose_terrain_render_policy(terrain_map_) != TerrainRenderPolicy::HeightTerrain) {
    return;
  }
  const MapData& map = terrain_map_;
  terrain_geometry_ = build_terrain_geometry(map.height_grid, map.ramps, map.tile_size);
  if (terrain_geometry_.tiles.empty()) {
    terrain_geometry_ = {};
    return;
  }

  const GreyboxFillMesh fill = build_greybox_fill_mesh(map, player_, has_player_);
  if (fill.vertices.empty()) {
    // Safe fallback: keep legacy floor/grid if geometry is invalid or exceeds uint16 indexing.
    terrain_geometry_ = {};
    return;
  }
  surface_ = build_surface_visual_mesh(fill,map.tile_size);

  auto push_vertex = [](std::vector<DebugColorVertex>& out, float x, float y, float z,
                        std::uint32_t color) {
    out.push_back(DebugColorVertex{x, y, z, color});
  };

  const std::uint32_t grid_color = 0xff8d8984;
  const std::uint32_t axis_x = 0xff858598;
  const std::uint32_t axis_z = 0xff988585;
  constexpr float kLineOffset = 0.03f;
  const auto lines = build_terrain_grid_lines(terrain_geometry_, kLineOffset);
  terrain_grid_line_data_.reserve(lines.size() * 2);
  for (const TerrainLineSegment& line : lines) {
    const bool axis_z_line = line.a.z == 0.0f && line.b.z == 0.0f;
    const bool axis_x_line = line.a.x == 0.0f && line.b.x == 0.0f;
    const std::uint32_t color = axis_z_line ? axis_z : (axis_x_line ? axis_x : grid_color);
    push_vertex(terrain_grid_line_data_, line.a.x, line.a.y, line.a.z, color);
    push_vertex(terrain_grid_line_data_, line.b.x, line.b.y, line.b.z, color);
  }
}

void GreyboxScene::set_selected_blocker(int index) {
  selected_blocker_ = index;
}

void GreyboxScene::set_selected_event_marker(int index) {
  selected_event_marker_ = index;
}

void GreyboxScene::set_camera_override(std::optional<ClimbCameraPose> pose) {
  camera_override_ = std::move(pose);
  has_display_ = false;
  rebuild_camera();
}

void GreyboxScene::rebuild_camera() {
  if (has_player_) {
    params_.focus = {player_.x, player_.y, player_.z};
  }
  camera_ = build_ortho_camera(width_, height_, params_);

  ClimbCameraPose target;
  Vec3 target_up;
  if (camera_override_ && !climb_locked_) {
    target = *camera_override_;
    target_up = {0.0f, 1.0f, 0.0f};
  } else if (climb_locked_) {
    target = climb_camera_pose({player_.x, player_.y, player_.z}, climb_into_x_, climb_into_z_);
    target_up = {0.0f, 1.0f, 0.0f};
  } else {
    target.eye = camera_.eye;
    target.focus = params_.focus;
    target_up = (params_.mode == CameraMode::TopDown) ? Vec3{0.0f, 0.0f, -1.0f}
                                                     : Vec3{0.0f, 1.0f, 0.0f};
  }

  if (!has_display_) {
    display_pose_ = target;
    from_pose_ = target;
    display_up_ = target_up;
    from_up_ = target_up;
    turn_t_ = 1.0f;
    has_display_ = true;
  } else {
    display_pose_ = lerp_climb_camera_pose(from_pose_, target, turn_t_);
    display_up_ = {from_up_.x + (target_up.x - from_up_.x) * turn_t_,
                   from_up_.y + (target_up.y - from_up_.y) * turn_t_,
                   from_up_.z + (target_up.z - from_up_.z) * turn_t_};
  }

  camera_.eye = display_pose_.eye;
  const bx::Vec3 eye{display_pose_.eye.x, display_pose_.eye.y, display_pose_.eye.z};
  const bx::Vec3 at{display_pose_.focus.x, display_pose_.focus.y, display_pose_.focus.z};
  const float up_len_sq =
      display_up_.x * display_up_.x + display_up_.y * display_up_.y + display_up_.z * display_up_.z;
  bx::Vec3 up{0.0f, 1.0f, 0.0f};
  if (up_len_sq > 1e-16f) {
    const float inv_len = 1.0f / std::sqrt(up_len_sq);
    up = {display_up_.x * inv_len, display_up_.y * inv_len, display_up_.z * inv_len};
  }
  bx::mtxLookAt(camera_.view.m, eye, at, up, bx::Handedness::Left);

  const bgfx::Caps* caps = bgfx::getCaps();
  if (caps != nullptr) {
    bx::mtxOrtho(camera_.proj.m, -camera_.half_width_world, camera_.half_width_world,
                 -camera_.half_height_world, camera_.half_height_world, params_.near_plane,
                 params_.far_plane, 0.0f, caps->homogeneousDepth);
  }
}

void GreyboxScene::draw(bgfx::ViewId view_id) {
  if (!initialized_) {
    return;
  }

  // Keep the camera centered on the player so the marker stays on screen.
  if (has_player_) {
    params_.focus = {player_.x, player_.y, player_.z};
    rebuild_camera();
  }

  bgfx::setViewName(view_id, "Greybox");
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x2a2c30ff, 1.0f, 0);
  bgfx::setViewRect(view_id, 0, 0, static_cast<uint16_t>(width_),
                    static_cast<uint16_t>(height_));
  bgfx::setViewTransform(view_id, camera_.view.m, camera_.proj.m);
  bgfx::setViewMode(view_id,bgfx::ViewMode::Sequential);
  bgfx::touch(view_id);

  constexpr float kHalf = 16.0f;
  const std::uint32_t floor_color = 0xff707070;
  const std::uint32_t grid_color = 0xff3a3a3a;
  const std::uint32_t axis_x = 0xff5050d0;
  const std::uint32_t axis_z = 0xffd05050;

  auto submit_tris = [&](const DebugColorVertex* verts, std::uint32_t num_verts,
                         const std::uint16_t* indices, std::uint32_t num_indices) {
    if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, layout_) ||
        num_indices != bgfx::getAvailTransientIndexBuffer(num_indices)) {
      return;
    }
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_);
    bgfx::allocTransientIndexBuffer(&tib, num_indices);
    bx::memCopy(tvb.data, verts, sizeof(DebugColorVertex) * num_verts);
    bx::memCopy(tib.data, indices, sizeof(std::uint16_t) * num_indices);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS);
    bgfx::submit(view_id, program_);
  };

  auto submit_lines = [&](const DebugColorVertex* verts, std::uint32_t num_verts) {
    if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, layout_)) {
      return;
    }
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_);
    bx::memCopy(tvb.data, verts, sizeof(DebugColorVertex) * num_verts);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_PT_LINES);
    bgfx::submit(view_id, program_);
  };

  auto submit_tris_packed =
      [&](const std::vector<SurfaceVertex>& verts, const std::vector<std::uint32_t>& indices) {
    const std::uint32_t num_verts = static_cast<std::uint32_t>(verts.size());
    const std::uint32_t num_indices = static_cast<std::uint32_t>(indices.size());
    if (num_verts == 0 || num_indices == 0) {
      return;
    }
    if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, surface_layout_) ||
        num_indices != bgfx::getAvailTransientIndexBuffer(num_indices,true)) {
      return;
    }
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, surface_layout_);
    bgfx::allocTransientIndexBuffer(&tib, num_indices,true);
    bx::memCopy(tvb.data, verts.data(), verts.size() * sizeof(SurfaceVertex));
    bx::memCopy(tib.data, indices.data(), indices.size() * sizeof(std::uint32_t));
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS);
    const float material[]={1.0f / std::max(terrain_map_.tile_size,1e-20f),0,0,0};
    bgfx::setUniform(surface_uniform_,material);
    bgfx::submit(view_id, surface_program_);
  };

  auto submit_lines_packed = [&](const std::vector<DebugColorVertex>& verts) {
    const std::uint32_t num_verts = static_cast<std::uint32_t>(verts.size());
    if (num_verts == 0) {
      return;
    }
    if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, layout_)) {
      return;
    }
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_);
    bx::memCopy(tvb.data, verts.data(), verts.size() * sizeof(DebugColorVertex));
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_PT_LINES);
    bgfx::submit(view_id, program_);
  };

  if (!surface_.vertices.empty() && !surface_.indices.empty()) {
    submit_tris_packed(surface_.vertices, surface_.indices);
    submit_lines_packed(terrain_grid_line_data_);
    std::vector<ContourVertex> vertices;
    std::vector<std::uint32_t> indices;
    vertices.reserve(surface_.contours.size()*4); indices.reserve(surface_.contours.size()*6);
    for(const auto& edge:surface_.contours) {
      const auto base=static_cast<std::uint32_t>(vertices.size());
      for(float side:{-1.0f,1.0f}) vertices.push_back({edge.a.x,edge.a.y,edge.a.z,edge.b.x,edge.b.y,edge.b.z,side,0});
      for(float side:{-1.0f,1.0f}) vertices.push_back({edge.b.x,edge.b.y,edge.b.z,edge.a.x,edge.a.y,edge.a.z,side,0});
      for(auto i:{0u,1u,2u,0u,2u,3u}) indices.push_back(base+i);
    }
    const auto nv=static_cast<std::uint32_t>(vertices.size()),ni=static_cast<std::uint32_t>(indices.size());
    if(nv && bgfx::getAvailTransientVertexBuffer(nv,contour_layout_)==nv && bgfx::getAvailTransientIndexBuffer(ni,true)==ni) {
      bgfx::TransientVertexBuffer vb; bgfx::TransientIndexBuffer ib;
      bgfx::allocTransientVertexBuffer(&vb,nv,contour_layout_);bgfx::allocTransientIndexBuffer(&ib,ni,true);
      bx::memCopy(vb.data,vertices.data(),vertices.size()*sizeof(ContourVertex));
      bx::memCopy(ib.data,indices.data(),indices.size()*sizeof(std::uint32_t));
      const float contour[]={1.0f/width_,1.0f/height_,1.5f,0.00001f};
      bgfx::setUniform(contour_uniform_,contour);
      bgfx::setVertexBuffer(0,&vb);bgfx::setIndexBuffer(&ib);
      bgfx::setState(BGFX_STATE_WRITE_RGB|BGFX_STATE_WRITE_A|BGFX_STATE_DEPTH_TEST_LEQUAL);
      bgfx::submit(view_id,contour_program_);
    }
  } else {
    const DebugColorVertex floor_verts[4] = {
        {-kHalf, 0.0f, -kHalf, floor_color},
        {kHalf, 0.0f, -kHalf, floor_color},
        {kHalf, 0.0f, kHalf, floor_color},
        {-kHalf, 0.0f, kHalf, floor_color},
    };
    const std::uint16_t floor_indices[6] = {0, 1, 2, 0, 2, 3};
    submit_tris(floor_verts, 4, floor_indices, 6);

    std::vector<DebugColorVertex> lines;
    lines.reserve(160);
    auto add_line = [&](float x0, float z0, float x1, float z1, std::uint32_t abgr) {
      lines.push_back({x0, 0.02f, z0, abgr});
      lines.push_back({x1, 0.02f, z1, abgr});
    };
    for (int i = -16; i <= 16; ++i) {
      const float f = static_cast<float>(i);
      add_line(-kHalf, f, kHalf, f, (i == 0) ? axis_z : grid_color);
      add_line(f, -kHalf, f, kHalf, (i == 0) ? axis_x : grid_color);
    }
    submit_lines(lines.data(), static_cast<std::uint32_t>(lines.size()));
  }

  const std::uint16_t quad_indices[6] = {0, 1, 2, 0, 2, 3};
  const std::uint32_t blocker_color = 0xff554080;
  const std::uint32_t blocker_jumpable_color = 0xffa068d0;
  const std::uint32_t selected_blocker_color = 0xff40c0ff;
  for (std::size_t i = 0; i < blockers_.size(); ++i) {
    const Aabb2& b = blockers_[i].bounds;
    const float center_x = 0.5f * (b.min_x + b.max_x);
    const float center_z = 0.5f * (b.min_z + b.max_z);
    const bool has_jump_heights = blockers_[i].jumpable && blockers_[i].base_y.has_value() &&
                                  blockers_[i].top_y.has_value();
    const float ground = sample_terrain_height(terrain_geometry_, center_x, center_z);
    const float base_y = has_jump_heights ? *blockers_[i].base_y : (ground + 0.04f);
    const float top_y = has_jump_heights ? *blockers_[i].top_y : (base_y + 0.9f);
    const std::uint32_t base_color = has_jump_heights ? blocker_jumpable_color : blocker_color;
    std::uint32_t color = base_color;
    if (has_player_ && has_terrain_map_ && static_cast<int>(i) != selected_blocker_) {
      color = greybox_fill_abgr(terrain_map_, player_, center_x, top_y, center_z, base_color);
    } else if (static_cast<int>(i) == selected_blocker_) {
      color = selected_blocker_color;
    }
    const DebugColorVertex verts[4] = {
        {b.min_x, top_y, b.min_z, color},
        {b.max_x, top_y, b.min_z, color},
        {b.max_x, top_y, b.max_z, color},
        {b.min_x, top_y, b.max_z, color},
    };
    submit_tris(verts, 4, quad_indices, 6);

    const DebugColorVertex outline[] = {
        {b.min_x, base_y, b.min_z, color}, {b.max_x, base_y, b.min_z, color},
        {b.max_x, base_y, b.min_z, color}, {b.max_x, base_y, b.max_z, color},
        {b.max_x, base_y, b.max_z, color}, {b.min_x, base_y, b.max_z, color},
        {b.min_x, base_y, b.max_z, color}, {b.min_x, base_y, b.min_z, color},
        {b.min_x, top_y, b.min_z, color},  {b.max_x, top_y, b.min_z, color},
        {b.max_x, top_y, b.min_z, color},  {b.max_x, top_y, b.max_z, color},
        {b.max_x, top_y, b.max_z, color},  {b.min_x, top_y, b.max_z, color},
        {b.min_x, top_y, b.max_z, color},  {b.min_x, top_y, b.min_z, color},
        {b.min_x, base_y, b.min_z, color}, {b.min_x, top_y, b.min_z, color},
        {b.max_x, base_y, b.min_z, color}, {b.max_x, top_y, b.min_z, color},
        {b.max_x, base_y, b.max_z, color}, {b.max_x, top_y, b.max_z, color},
        {b.min_x, base_y, b.max_z, color}, {b.min_x, top_y, b.max_z, color},
    };
    submit_lines(outline, static_cast<std::uint32_t>(std::size(outline)));
  }

  // Cyan pillars mark interactive events (NPC / scrap).
  const std::uint32_t event_color = 0xffe0c040;
  const std::uint32_t selected_event_color = 0xff40ffff;
  for (std::size_t i = 0; i < event_markers_.size(); ++i) {
    const Vec3& m = event_markers_[i];
    const std::uint32_t color =
        (static_cast<int>(i) == selected_event_marker_) ? selected_event_color : event_color;
    const float h = 0.35f;
    const float marker_base_y = m.y + 0.06f;
    const DebugColorVertex base[4] = {
        {m.x - h, marker_base_y, m.z - h, color},
        {m.x + h, marker_base_y, m.z - h, color},
        {m.x + h, marker_base_y, m.z + h, color},
        {m.x - h, marker_base_y, m.z + h, color},
    };
    submit_tris(base, 4, quad_indices, 6);
    const DebugColorVertex stem[2] = {
        {m.x, marker_base_y, m.z, color},
        {m.x, m.y + 1.4f, m.z, color},
    };
    submit_lines(stem, 2);
  }

  if (has_player_) {
    const float h = player_.half_extent;
    const float body_y = player_.y + 0.08f;
    const float head_y = player_.y + 1.6f;
    const std::uint32_t player_color = 0xff30e070;
    const DebugColorVertex body[4] = {
        {player_.x - h, body_y, player_.z - h, player_color},
        {player_.x + h, body_y, player_.z - h, player_color},
        {player_.x + h, body_y, player_.z + h, player_color},
        {player_.x - h, body_y, player_.z + h, player_color},
    };
    submit_tris(body, 4, quad_indices, 6);

    // Tall marker so the player reads clearly under ortho 3/4.
    const DebugColorVertex stem[2] = {
        {player_.x, body_y, player_.z, 0xffe8ffe8},
        {player_.x, head_y, player_.z, 0xffe8ffe8},
    };
    submit_lines(stem, 2);

    const DebugColorVertex head[4] = {
        {player_.x - 0.2f, head_y, player_.z - 0.2f, 0xff90ffb0},
        {player_.x + 0.2f, head_y, player_.z - 0.2f, 0xff90ffb0},
        {player_.x + 0.2f, head_y, player_.z + 0.2f, 0xff90ffb0},
        {player_.x - 0.2f, head_y, player_.z + 0.2f, 0xff90ffb0},
    };
    submit_tris(head, 4, quad_indices, 6);
  }
}

}  // namespace rat
