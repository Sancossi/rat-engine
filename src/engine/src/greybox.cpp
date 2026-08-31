#include "rat/greybox.hpp"

#include <bgfx/embedded_shader.h>
#include <bx/bx.h>
#include <bx/math.h>

#include <vector>

#include "fs_debugdraw_lines.bin.h"
#include "vs_debugdraw_lines.bin.h"

namespace rat {
namespace {

// Position + Color0 only — matches vs/fs_debugdraw_lines (u_modelViewProj + vertex color).
// Do NOT use vs_debugdraw_fill here: that shader expects a_indices and u_matColor.
const bgfx::EmbeddedShader k_shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_debugdraw_lines),
    BGFX_EMBEDDED_SHADER(fs_debugdraw_lines),
    BGFX_EMBEDDED_SHADER_END()};

struct ColorVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  std::uint32_t abgr = 0xffffffff;
};

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

  const bgfx::RendererType::Enum type = bgfx::getRendererType();
  program_ = bgfx::createProgram(
      bgfx::createEmbeddedShader(k_shaders, type, "vs_debugdraw_lines"),
      bgfx::createEmbeddedShader(k_shaders, type, "fs_debugdraw_lines"), true);

  if (!bgfx::isValid(program_)) {
    shutdown();
    return false;
  }

  rebuild_camera();
  initialized_ = true;
  return true;
}

void GreyboxScene::shutdown() {
  destroy_program(program_);
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

void GreyboxScene::set_blockers(std::span<const Aabb2> blockers) {
  blockers_.assign(blockers.begin(), blockers.end());
}

void GreyboxScene::set_event_markers(std::span<const Vec3> markers) {
  event_markers_.assign(markers.begin(), markers.end());
}

void GreyboxScene::rebuild_camera() {
  camera_ = build_ortho_three_quarter(width_, height_, params_);

  // Use bx matrices so handedness/depth match the active renderer.
  const bx::Vec3 eye{camera_.eye.x, camera_.eye.y, camera_.eye.z};
  const bx::Vec3 at{params_.focus.x, params_.focus.y, params_.focus.z};
  const bx::Vec3 up{0.0f, 1.0f, 0.0f};
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
    params_.focus = {player_.x, 0.0f, player_.z};
    rebuild_camera();
  }

  bgfx::setViewName(view_id, "Greybox");
  bgfx::setViewClear(view_id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x2a2c30ff, 1.0f, 0);
  bgfx::setViewRect(view_id, 0, 0, static_cast<uint16_t>(width_),
                    static_cast<uint16_t>(height_));
  bgfx::setViewTransform(view_id, camera_.view.m, camera_.proj.m);
  bgfx::touch(view_id);

  constexpr float kHalf = 16.0f;
  const std::uint32_t floor_color = 0xff707070;
  const std::uint32_t grid_color = 0xff3a3a3a;
  const std::uint32_t axis_x = 0xff5050d0;
  const std::uint32_t axis_z = 0xffd05050;

  auto submit_tris = [&](const ColorVertex* verts, std::uint32_t num_verts,
                         const std::uint16_t* indices, std::uint32_t num_indices) {
    if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, layout_) ||
        num_indices != bgfx::getAvailTransientIndexBuffer(num_indices)) {
      return;
    }
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_);
    bgfx::allocTransientIndexBuffer(&tib, num_indices);
    bx::memCopy(tvb.data, verts, sizeof(ColorVertex) * num_verts);
    bx::memCopy(tib.data, indices, sizeof(std::uint16_t) * num_indices);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS);
    bgfx::submit(view_id, program_);
  };

  auto submit_lines = [&](const ColorVertex* verts, std::uint32_t num_verts) {
    if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, layout_)) {
      return;
    }
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_);
    bx::memCopy(tvb.data, verts, sizeof(ColorVertex) * num_verts);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_PT_LINES);
    bgfx::submit(view_id, program_);
  };

  {
    const ColorVertex floor_verts[4] = {
        {-kHalf, 0.0f, -kHalf, floor_color},
        {kHalf, 0.0f, -kHalf, floor_color},
        {kHalf, 0.0f, kHalf, floor_color},
        {-kHalf, 0.0f, kHalf, floor_color},
    };
    const std::uint16_t floor_indices[6] = {0, 1, 2, 0, 2, 3};
    submit_tris(floor_verts, 4, floor_indices, 6);
  }

  std::vector<ColorVertex> lines;
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

  const std::uint16_t quad_indices[6] = {0, 1, 2, 0, 2, 3};
  const std::uint32_t blocker_color = 0xff554080;
  for (const Aabb2& b : blockers_) {
    const ColorVertex verts[4] = {
        {b.min_x, 0.04f, b.min_z, blocker_color},
        {b.max_x, 0.04f, b.min_z, blocker_color},
        {b.max_x, 0.04f, b.max_z, blocker_color},
        {b.min_x, 0.04f, b.max_z, blocker_color},
    };
    submit_tris(verts, 4, quad_indices, 6);
  }

  // Cyan pillars mark interactive events (NPC / scrap).
  const std::uint32_t event_color = 0xffe0c040;
  for (const Vec3& m : event_markers_) {
    const float h = 0.35f;
    const ColorVertex base[4] = {
        {m.x - h, 0.06f, m.z - h, event_color},
        {m.x + h, 0.06f, m.z - h, event_color},
        {m.x + h, 0.06f, m.z + h, event_color},
        {m.x - h, 0.06f, m.z + h, event_color},
    };
    submit_tris(base, 4, quad_indices, 6);
    const ColorVertex stem[2] = {
        {m.x, 0.06f, m.z, 0xffffe080},
        {m.x, 1.4f, m.z, 0xffffe080},
    };
    submit_lines(stem, 2);
  }

  if (has_player_) {
    const float h = player_.half_extent;
    const std::uint32_t player_color = 0xff30e070;
    const ColorVertex body[4] = {
        {player_.x - h, 0.08f, player_.z - h, player_color},
        {player_.x + h, 0.08f, player_.z - h, player_color},
        {player_.x + h, 0.08f, player_.z + h, player_color},
        {player_.x - h, 0.08f, player_.z + h, player_color},
    };
    submit_tris(body, 4, quad_indices, 6);

    // Tall marker so the player reads clearly under ortho 3/4.
    const ColorVertex stem[2] = {
        {player_.x, 0.08f, player_.z, 0xffe8ffe8},
        {player_.x, 1.6f, player_.z, 0xffe8ffe8},
    };
    submit_lines(stem, 2);

    const ColorVertex head[4] = {
        {player_.x - 0.2f, 1.6f, player_.z - 0.2f, 0xff90ffb0},
        {player_.x + 0.2f, 1.6f, player_.z - 0.2f, 0xff90ffb0},
        {player_.x + 0.2f, 1.6f, player_.z + 0.2f, 0xff90ffb0},
        {player_.x - 0.2f, 1.6f, player_.z + 0.2f, 0xff90ffb0},
    };
    submit_tris(head, 4, quad_indices, 6);
  }
}

}  // namespace rat
