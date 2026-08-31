#include "rat/greybox.hpp"

#include <bgfx/embedded_shader.h>
#include <bx/bx.h>
#include <bx/math.h>

#include <vector>

#include "fs_debugdraw_fill.bin.h"
#include "fs_debugdraw_lines.bin.h"
#include "vs_debugdraw_fill.bin.h"
#include "vs_debugdraw_lines.bin.h"

namespace rat {
namespace {

const bgfx::EmbeddedShader k_shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_debugdraw_lines),
    BGFX_EMBEDDED_SHADER(fs_debugdraw_lines),
    BGFX_EMBEDDED_SHADER(vs_debugdraw_fill),
    BGFX_EMBEDDED_SHADER(fs_debugdraw_fill),
    BGFX_EMBEDDED_SHADER_END()};

struct LineVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float len = 0.0f;
  std::uint32_t abgr = 0xffffffff;
};

struct FillVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float u = 0.0f;
  float v = 0.0f;
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

  layout_lines_.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 1, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();

  layout_fill_.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();

  const bgfx::RendererType::Enum type = bgfx::getRendererType();
  program_lines_ = bgfx::createProgram(
      bgfx::createEmbeddedShader(k_shaders, type, "vs_debugdraw_lines"),
      bgfx::createEmbeddedShader(k_shaders, type, "fs_debugdraw_lines"), true);
  program_fill_ = bgfx::createProgram(
      bgfx::createEmbeddedShader(k_shaders, type, "vs_debugdraw_fill"),
      bgfx::createEmbeddedShader(k_shaders, type, "fs_debugdraw_fill"), true);

  if (!bgfx::isValid(program_lines_) || !bgfx::isValid(program_fill_)) {
    shutdown();
    return false;
  }

  rebuild_camera();
  initialized_ = true;
  return true;
}

void GreyboxScene::shutdown() {
  destroy_program(program_lines_);
  destroy_program(program_fill_);
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

void GreyboxScene::rebuild_camera() {
  camera_ = build_ortho_three_quarter(width_, height_, params_);

  // Rebuild projection with bx so depth matches the active renderer caps.
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

  bgfx::setViewName(view_id, "Greybox");
  bgfx::setViewRect(view_id, 0, 0, static_cast<uint16_t>(width_),
                    static_cast<uint16_t>(height_));
  bgfx::setViewTransform(view_id, camera_.view.m, camera_.proj.m);
  bgfx::touch(view_id);

  constexpr float kHalf = 16.0f;
  constexpr float kY = 0.0f;
  const std::uint32_t floor_color = 0xff6e6e6e;
  const std::uint32_t grid_color = 0xff4a4a4a;
  const std::uint32_t axis_x = 0xff5050c0;
  const std::uint32_t axis_z = 0xffc05050;

  {
    FillVertex verts[4] = {
        {-kHalf, kY, -kHalf, 0.0f, 0.0f, floor_color},
        {kHalf, kY, -kHalf, 1.0f, 0.0f, floor_color},
        {kHalf, kY, kHalf, 1.0f, 1.0f, floor_color},
        {-kHalf, kY, kHalf, 0.0f, 1.0f, floor_color},
    };
    const std::uint16_t indices[6] = {0, 1, 2, 0, 2, 3};

    if (4 == bgfx::getAvailTransientVertexBuffer(4, layout_fill_) &&
        6 == bgfx::getAvailTransientIndexBuffer(6)) {
      bgfx::TransientVertexBuffer tvb;
      bgfx::TransientIndexBuffer tib;
      bgfx::allocTransientVertexBuffer(&tvb, 4, layout_fill_);
      bgfx::allocTransientIndexBuffer(&tib, 6);
      bx::memCopy(tvb.data, verts, sizeof(verts));
      bx::memCopy(tib.data, indices, sizeof(indices));
      bgfx::setVertexBuffer(0, &tvb);
      bgfx::setIndexBuffer(&tib);
      bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                     BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
      bgfx::submit(view_id, program_fill_);
    }
  }

  std::vector<LineVertex> lines;
  lines.reserve(128);
  auto add_line = [&](float x0, float z0, float x1, float z1, std::uint32_t abgr) {
    lines.push_back({x0, 0.01f, z0, 0.0f, abgr});
    lines.push_back({x1, 0.01f, z1, 1.0f, abgr});
  };

  for (int i = -16; i <= 16; ++i) {
    const float f = static_cast<float>(i);
    const std::uint32_t color = (i == 0) ? axis_z : grid_color;
    add_line(-kHalf, f, kHalf, f, color);
    const std::uint32_t color_x = (i == 0) ? axis_x : grid_color;
    add_line(f, -kHalf, f, kHalf, color_x);
  }

  const std::uint32_t num_verts = static_cast<std::uint32_t>(lines.size());
  if (num_verts == 0) {
    return;
  }
  if (num_verts != bgfx::getAvailTransientVertexBuffer(num_verts, layout_lines_)) {
    return;
  }

  bgfx::TransientVertexBuffer tvb;
  bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_lines_);
  bx::memCopy(tvb.data, lines.data(), lines.size() * sizeof(LineVertex));
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                 BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_PT_LINES);
  bgfx::submit(view_id, program_lines_);
}

}  // namespace rat
