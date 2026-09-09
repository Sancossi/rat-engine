#include "rat/sprite_renderer.hpp"
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>
#include "vs_sprite_glsl.bin.h"
#include "fs_sprite_glsl.bin.h"
#include "vs_sprite_spirv.bin.h"
#include "fs_sprite_spirv.bin.h"
#ifdef _WIN32
#include "vs_sprite_dxbc.bin.h"
#include "fs_sprite_dxbc.bin.h"
#endif
namespace rat {
namespace {
struct Vertex { float x,y,z,u,v; std::uint32_t color; };
bgfx::ProgramHandle make_program() {
  const auto make = [](const auto& vs, const auto& fs) {
    return bgfx::createProgram(bgfx::createShader(bgfx::copy(vs, sizeof(vs))), bgfx::createShader(bgfx::copy(fs, sizeof(fs))), true);
  };
  switch (bgfx::getRendererType()) {
    case bgfx::RendererType::OpenGL: return make(vs_sprite_glsl, fs_sprite_glsl);
    case bgfx::RendererType::Vulkan: return make(vs_sprite_spirv, fs_sprite_spirv);
#ifdef _WIN32
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12: return make(vs_sprite_dxbc, fs_sprite_dxbc);
#endif
    default: return BGFX_INVALID_HANDLE;
  }
}
}
bool SpriteRenderer::init(const std::filesystem::path& png, std::string& error) {
  shutdown();
  std::ifstream input(png, std::ios::binary | std::ios::ate);
  if (!input) { error = "Cannot open sprite PNG: " + png.generic_string(); return false; }
  const auto size = input.tellg();
  if (size < 24 || size > 1024 * 1024) { error = "Invalid prototype PNG file size"; return false; }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  input.seekg(0);
  if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) { error = "Cannot read sprite PNG"; return false; }
  constexpr std::array<std::uint8_t, 8> signature{137,80,78,71,13,10,26,10};
  const auto be32 = [&](int offset) { return (std::uint32_t(bytes[offset]) << 24) | (std::uint32_t(bytes[offset+1]) << 16) | (std::uint32_t(bytes[offset+2]) << 8) | bytes[offset+3]; };
  if (!std::equal(signature.begin(), signature.end(), bytes.begin()) || be32(16) != 64 || be32(20) != 192) {
    error = "Prototype rat atlas must be a 64x192 PNG (32x48, two frames, four directions)"; return false;
  }
  bx::DefaultAllocator allocator;
  auto* image = bimg::imageParse(&allocator, bytes.data(), static_cast<std::uint32_t>(bytes.size()), bimg::TextureFormat::RGBA8);
  if (!image) { error = "Cannot decode sprite PNG: " + png.generic_string(); return false; }
  if (image->m_width!=64 || image->m_height!=192 || image->m_depth!=1 || image->m_size!=64*192*4 || image->m_cubeMap) {
    bimg::imageFree(image); error="Decoded rat atlas does not match 64x192 RGBA8"; return false;
  }
  texture_ = bgfx::createTexture2D(64, 192, false, 1, bgfx::TextureFormat::RGBA8,
      BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
      bgfx::copy(image->m_data, image->m_size));
  bimg::imageFree(image);
  program_ = make_program();
  sampler_ = bgfx::createUniform("s_atlas", bgfx::UniformType::Sampler);
  layout_.begin().add(bgfx::Attrib::Position,3,bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0,2,bgfx::AttribType::Float).add(bgfx::Attrib::Color0,4,bgfx::AttribType::Uint8,true).end();
  if (!bgfx::isValid(texture_) || !bgfx::isValid(program_) || !bgfx::isValid(sampler_)) {
    error = "Cannot create sprite GPU resources"; shutdown(); return false;
  }
  return true;
}
void SpriteRenderer::shutdown() {
  if (bgfx::isValid(texture_)) bgfx::destroy(texture_);
  if (bgfx::isValid(program_)) bgfx::destroy(program_);
  if (bgfx::isValid(sampler_)) bgfx::destroy(sampler_);
  texture_ = BGFX_INVALID_HANDLE; program_ = BGFX_INVALID_HANDLE; sampler_ = BGFX_INVALID_HANDLE;
}
void SpriteRenderer::draw(const OrthoCamera& camera, Vec3 feet, int direction, int frame, bgfx::ViewId view, std::uint32_t tint, float height) {
  if (!bgfx::isValid(program_)) return;
  if (bgfx::getAvailTransientVertexBuffer(4, layout_) != 4 || bgfx::getAvailTransientIndexBuffer(6) != 6) return;
  // The orthographic view rows are world-space screen right/up. Snap only the display anchor.
  const auto& m = camera.view.m;
  const Vec3 right{m[0],m[4],m[8]}, up{m[1],m[5],m[9]};
  const float pixel = camera.half_height_world * 2 / 360.0f;
  const float sx = feet.x*right.x + feet.y*right.y + feet.z*right.z + m[12];
  const float sy = feet.x*up.x + feet.y*up.y + feet.z*up.z + m[13];
  const float dx = std::round(sx / pixel)*pixel-sx, dy = std::round(sy/pixel)*pixel-sy;
  feet.x += dx*right.x + dy*up.x; feet.y += dx*right.y + dy*up.y; feet.z += dx*right.z + dy*up.z;
  const float width = height * 32.0f/48.0f;
  // Source foot baseline is row 43. Five transparent rows extend below the anchor.
  const float bottom = -height * 5.0f / 48.0f, top = height + bottom;
  const float u0 = std::clamp(frame,0,1)*0.5f, u1 = u0+0.5f;
  const float v0 = std::clamp(direction,0,3)*0.25f, v1 = v0+0.25f;
  const auto vertex = [&](float x,float y,float u,float v) { return Vertex{feet.x+right.x*x+up.x*y, feet.y+right.y*x+up.y*y, feet.z+right.z*x+up.z*y,u,v,tint}; };
  const std::array<Vertex,4> vertices{vertex(-width/2,bottom,u0,v1),vertex(width/2,bottom,u1,v1),vertex(width/2,top,u1,v0),vertex(-width/2,top,u0,v0)};
  constexpr std::array<std::uint16_t,6> indices{0,1,2,0,2,3};
  bgfx::TransientVertexBuffer vb; bgfx::TransientIndexBuffer ib;
  bgfx::allocTransientVertexBuffer(&vb,4,layout_); bgfx::allocTransientIndexBuffer(&ib,6);
  std::memcpy(vb.data,vertices.data(),sizeof(vertices)); std::memcpy(ib.data,indices.data(),sizeof(indices));
  bgfx::setTransform(nullptr);
  bgfx::setVertexBuffer(0,&vb); bgfx::setIndexBuffer(&ib); bgfx::setTexture(0,sampler_,texture_);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);
  bgfx::submit(view,program_);
}
}
