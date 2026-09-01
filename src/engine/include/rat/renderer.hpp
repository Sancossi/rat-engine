#pragma once

#include <cstdint>

namespace rat {

struct RenderWorld;

struct NativeWindowHandle {
  void* nwh = nullptr;
  void* ndt = nullptr; // display / native display type (unused on Win32)
};

struct RendererConfig {
  NativeWindowHandle window;
  std::uint32_t width = 1;
  std::uint32_t height = 1;
  bool vsync = true;
};

class Renderer {
 public:
  Renderer() = default;
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  bool init(const RendererConfig& config);
  void shutdown();
  void resize(std::uint32_t width, std::uint32_t height);
  void begin_frame();
  void submit(const RenderWorld& world);
  void end_frame();

  [[nodiscard]] bool is_initialized() const { return initialized_; }

 private:
  bool initialized_ = false;
  std::uint32_t width_ = 0;
  std::uint32_t height_ = 0;
  std::uint32_t reset_flags_ = 0;
};

}  // namespace rat
