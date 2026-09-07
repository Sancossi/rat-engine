#include "imgui_bgfx.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>

#include <imgui.h>

#include <cstring>

#include "fs_ocornut_imgui.bin.h"
#include "vs_ocornut_imgui.bin.h"

namespace rat::imgui_bgfx {
namespace {

static const bgfx::EmbeddedShader k_embedded_shaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER_END()};

bgfx::ProgramHandle program_ = BGFX_INVALID_HANDLE;
bgfx::UniformHandle texture_uniform_ = BGFX_INVALID_HANDLE;
bgfx::TextureHandle font_texture_ = BGFX_INVALID_HANDLE;
bgfx::VertexLayout layout_;
bgfx::ViewId view_id_ = 255;
bool initialized_ = false;

bool create_fonts_texture() {
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels = nullptr;
  int width = 0;
  int height = 0;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  if (pixels == nullptr || width <= 0 || height <= 0) {
    return false;
  }

  const bgfx::Memory* mem = bgfx::copy(pixels, width * height * 4);
  font_texture_ = bgfx::createTexture2D(
      static_cast<uint16_t>(width),
      static_cast<uint16_t>(height),
      false,
      1,
      bgfx::TextureFormat::RGBA8,
      0,
      mem);

  io.Fonts->SetTexID(
      static_cast<ImTextureID>(static_cast<uintptr_t>(font_texture_.idx)));
  return bgfx::isValid(font_texture_);
}

bool check_avail_transient(uint32_t num_verts, uint32_t num_indices) {
  return num_verts == bgfx::getAvailTransientVertexBuffer(num_verts, layout_) &&
         num_indices ==
             bgfx::getAvailTransientIndexBuffer(num_indices, sizeof(ImDrawIdx) == 4);
}

void render_draw_data(ImDrawData* draw_data) {
  if (draw_data == nullptr || draw_data->CmdListsCount == 0) {
    return;
  }

  const int fb_width =
      static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
  const int fb_height =
      static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
  if (fb_width <= 0 || fb_height <= 0) {
    return;
  }

  bgfx::setViewName(view_id_, "ImGui");
  bgfx::setViewMode(view_id_, bgfx::ViewMode::Sequential);

  const bgfx::Caps* caps = bgfx::getCaps();
  float ortho[16];
  const float x = draw_data->DisplayPos.x;
  const float y = draw_data->DisplayPos.y;
  const float width = draw_data->DisplaySize.x;
  const float height = draw_data->DisplaySize.y;
  bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f,
               caps->homogeneousDepth);
  bgfx::setViewTransform(view_id_, nullptr, ortho);
  bgfx::setViewRect(view_id_, 0, 0, static_cast<uint16_t>(fb_width),
                    static_cast<uint16_t>(fb_height));

  const ImVec2 clip_pos = draw_data->DisplayPos;
  const ImVec2 clip_scale = draw_data->FramebufferScale;

  for (int n = 0; n < draw_data->CmdListsCount; ++n) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const uint32_t num_verts = static_cast<uint32_t>(cmd_list->VtxBuffer.Size);
    const uint32_t num_indices = static_cast<uint32_t>(cmd_list->IdxBuffer.Size);

    if (!check_avail_transient(num_verts, num_indices)) {
      break;
    }

    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientVertexBuffer(&tvb, num_verts, layout_);
    bgfx::allocTransientIndexBuffer(&tib, num_indices, sizeof(ImDrawIdx) == 4);

    memcpy(tvb.data, cmd_list->VtxBuffer.Data, num_verts * sizeof(ImDrawVert));
    memcpy(tib.data, cmd_list->IdxBuffer.Data, num_indices * sizeof(ImDrawIdx));

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i) {
      const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_i];
      if (cmd.UserCallback != nullptr) {
        cmd.UserCallback(cmd_list, &cmd);
        continue;
      }

      ImVec4 clip_rect;
      clip_rect.x = (cmd.ClipRect.x - clip_pos.x) * clip_scale.x;
      clip_rect.y = (cmd.ClipRect.y - clip_pos.y) * clip_scale.y;
      clip_rect.z = (cmd.ClipRect.z - clip_pos.x) * clip_scale.x;
      clip_rect.w = (cmd.ClipRect.w - clip_pos.y) * clip_scale.y;

      if (clip_rect.x < fb_width && clip_rect.y < fb_height &&
          clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
        const uint16_t xx = static_cast<uint16_t>(clip_rect.x > 0.0f ? clip_rect.x : 0.0f);
        const uint16_t yy = static_cast<uint16_t>(clip_rect.y > 0.0f ? clip_rect.y : 0.0f);
        bgfx::setScissor(
            xx, yy,
            static_cast<uint16_t>(clip_rect.z - xx),
            static_cast<uint16_t>(clip_rect.w - yy));

        bgfx::TextureHandle texture = font_texture_;
        if (cmd.GetTexID() != 0) {
          texture.idx = static_cast<uint16_t>(
              static_cast<uintptr_t>(cmd.GetTexID()) & 0xffff);
        }

        constexpr uint64_t state =
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA |
            BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                  BGFX_STATE_BLEND_INV_SRC_ALPHA);

        bgfx::setState(state);
        bgfx::setTexture(0, texture_uniform_, texture);
        bgfx::setVertexBuffer(0, &tvb, 0, num_verts);
        bgfx::setIndexBuffer(&tib, cmd.IdxOffset, cmd.ElemCount);
        bgfx::submit(view_id_, program_);
      }
    }
  }
}

}  // namespace

bool init(int view_id) {
  if (initialized_) {
    return true;
  }

  view_id_ = static_cast<bgfx::ViewId>(view_id);

  const bgfx::RendererType::Enum type = bgfx::getRendererType();
  program_ = bgfx::createProgram(
      bgfx::createEmbeddedShader(k_embedded_shaders, type, "vs_ocornut_imgui"),
      bgfx::createEmbeddedShader(k_embedded_shaders, type, "fs_ocornut_imgui"),
      true);
  if (!bgfx::isValid(program_)) {
    return false;
  }

  texture_uniform_ = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

  layout_.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();

  if (!create_fonts_texture()) {
    shutdown();
    return false;
  }

  initialized_ = true;
  return true;
}

void shutdown() {
  if (bgfx::isValid(font_texture_)) {
    bgfx::destroy(font_texture_);
    font_texture_ = BGFX_INVALID_HANDLE;
  }
  if (bgfx::isValid(texture_uniform_)) {
    bgfx::destroy(texture_uniform_);
    texture_uniform_ = BGFX_INVALID_HANDLE;
  }
  if (bgfx::isValid(program_)) {
    bgfx::destroy(program_);
    program_ = BGFX_INVALID_HANDLE;
  }
  initialized_ = false;
}

void begin_frame() { ImGui::NewFrame(); }

void end_frame() {
  ImGui::Render();
  if (initialized_) {
    render_draw_data(ImGui::GetDrawData());
  }
}

}  // namespace rat::imgui_bgfx
