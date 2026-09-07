#include "rat/renderer.hpp"

#include "rat/render_world.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/readerwriter.h>
#include <bx/error.h>
#include <bimg/bimg.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <cstdlib>

namespace rat {

struct Renderer::Callbacks final : bgfx::CallbackI {
  mutable std::mutex mutex;
  CaptureResult capture;
  void fatal(const char* file, uint16_t line, bgfx::Fatal::Enum, const char* message) override {
    std::fprintf(stderr, "bgfx fatal %s:%u: %s\n", file, line, message);
    std::abort();
  }
  void traceVargs(const char*, uint16_t, const char* format, va_list args) override { std::vfprintf(stderr, format, args); }
  void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
  void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
  void profilerEnd() override {}
  uint32_t cacheReadSize(uint64_t) override { return 0; }
  bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
  void cacheWrite(uint64_t, const void*, uint32_t) override {}
  void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
  void captureEnd() override {}
  void captureFrame(const void*, uint32_t) override {}
  void screenShot(const char* path, uint32_t width, uint32_t height, uint32_t pitch,
                  const void* data, uint32_t size, bool yflip) override {
    struct Writer final : bx::WriterI {
      std::ofstream stream;
      explicit Writer(const char* filename)
          : stream(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(filename))), std::ios::binary) {}
      int32_t write(const void* bytes, int32_t count, bx::Error*) override {
        stream.write(static_cast<const char*>(bytes), count);
        return stream ? count : 0;
      }
    } writer(path);
    bx::Error error;
    const bool valid = width > 0 && width < 16384 && height > 0 &&
        static_cast<uint64_t>(pitch) * height <= size && pitch >= static_cast<uint64_t>(width) * 4;
    bool ok = valid && writer.stream &&
        bimg::imageWritePng(&writer, width, height, pitch, data, bimg::TextureFormat::BGRA8, yflip, &error) > 0;
    writer.stream.flush();
    ok = ok && writer.stream && error.isOk();
    std::lock_guard lock(mutex);
    capture = {true, ok, path, ok ? "" : "PNG capture failed"};
  }
};

Renderer::Renderer() : callbacks_(std::make_unique<Callbacks>()) {}

void Renderer::request_capture(const std::string& path) {
  { std::lock_guard lock(callbacks_->mutex); callbacks_->capture = {false, false, path, ""}; }
  if (initialized_) bgfx::requestScreenShot(BGFX_INVALID_HANDLE, path.c_str());
}
CaptureResult Renderer::capture_result() const {
  std::lock_guard lock(callbacks_->mutex); return callbacks_->capture;
}
std::string Renderer::backend_name() const {
  if (!initialized_) return "uninitialized";
  std::string name = bgfx::getRendererName(bgfx::getRendererType());
  if (mode_ == RendererMode::SoftwareD3D11) name += " / WARP";
  if (mode_ == RendererMode::SoftwareOpenGL) name += " / software OpenGL requested";
  return name;
}

Renderer::~Renderer() {
  shutdown();
}

bool Renderer::init(const RendererConfig& config) {
  if (initialized_) {
    return true;
  }
  if (config.window.nwh == nullptr) {
    return false;
  }

  mode_ = config.mode;
  width_ = config.width < 1 ? 1 : config.width;
  height_ = config.height < 1 ? 1 : config.height;
  reset_flags_ = config.vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;

  // The editor owns the frame clock and drives bgfx on its UI thread.
  bgfx::renderFrame();

  bgfx::Init init;
  init.type = bgfx::RendererType::Count;  // auto-select (D3D11 preferred on Win)
#if BX_PLATFORM_WINDOWS
  init.type = bgfx::RendererType::Direct3D11;
#endif
  if (config.mode == RendererMode::SoftwareD3D11) {
    init.type = bgfx::RendererType::Direct3D11;
    init.vendorId = BGFX_PCI_ID_SOFTWARE_RASTERIZER;
  } else if (config.mode == RendererMode::SoftwareOpenGL) {
    init.type = bgfx::RendererType::OpenGL;
  }
  init.callback = callbacks_.get();
  init.platformData.nwh = config.window.nwh;
  init.platformData.ndt = config.window.ndt;
  init.resolution.width = width_;
  init.resolution.height = height_;
  init.resolution.reset = reset_flags_;

  if (!bgfx::init(init)) {
    return false;
  }

  bgfx::setDebug(BGFX_DEBUG_TEXT);
  // Cool grey backdrop behind the greybox floor.
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x2a2c30ff, 1.0f, 0);
  bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width_), static_cast<uint16_t>(height_));

  initialized_ = true;
  return true;
}

void Renderer::shutdown() {
  if (!initialized_) {
    return;
  }
  bgfx::shutdown();
  initialized_ = false;
}

void Renderer::resize(std::uint32_t width, std::uint32_t height) {
  if (!initialized_) {
    return;
  }
  width_ = width < 1 ? 1 : width;
  height_ = height < 1 ? 1 : height;
  bgfx::reset(width_, height_, reset_flags_);
  bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(width_), static_cast<uint16_t>(height_));
}

void Renderer::begin_frame() {
  if (!initialized_) {
    return;
  }
  bgfx::touch(0);
}

void Renderer::submit(const RenderWorld& world) {
  if (!initialized_) {
    return;
  }
  for (std::uint8_t i = 0; i < kRenderPassCount; ++i) {
    const auto pass = static_cast<RenderPass>(i);
    const bgfx::ViewId view = static_cast<bgfx::ViewId>(render_pass_view_id(pass));
    const char* name =
        i < world.pass_names.size() ? world.pass_names[i].c_str() : render_pass_name(pass);
    bgfx::setViewName(view, name);
    bgfx::setViewRect(view, 0, 0, static_cast<uint16_t>(width_), static_cast<uint16_t>(height_));
    bgfx::setMarker(name);
    bgfx::touch(view);
  }
  for (const RenderPacket& packet : world.packets) {
    const bgfx::ViewId view = static_cast<bgfx::ViewId>(render_pass_view_id(packet.pass));
    if (packet.mesh.valid()) {
      bgfx::setMarker(packet.mesh.key().c_str());
    }
    bgfx::touch(view);
  }
}

void Renderer::end_frame() {
  if (!initialized_) {
    return;
  }
  bgfx::frame();
}

}  // namespace rat
