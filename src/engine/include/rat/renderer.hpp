#pragma once

#include "rat/native_window_handle.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace rat {

struct RenderWorld;

enum class RendererMode { Auto, SoftwareD3D11, SoftwareOpenGL };

struct CaptureResult { bool complete = false; bool ok = false; std::string path; std::string error; };

struct RendererConfig {
  RendererMode mode = RendererMode::Auto;
  NativeWindowHandle window;
  std::uint32_t width = 1;
  std::uint32_t height = 1;
  bool vsync = true;
};

class Renderer {
 public:
  Renderer();
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  bool init(const RendererConfig& config);
  void shutdown();
  void resize(std::uint32_t width, std::uint32_t height);
  void begin_frame();
  void submit(const RenderWorld& world);
  void end_frame();
  void request_capture(const std::string& path);
  [[nodiscard]] CaptureResult capture_result() const;
  [[nodiscard]] std::string backend_name() const;

  [[nodiscard]] bool is_initialized() const { return initialized_; }

 private:
  struct Callbacks;
  std::unique_ptr<Callbacks> callbacks_;
  RendererMode mode_ = RendererMode::Auto;
  bool initialized_ = false;
  std::uint32_t width_ = 0;
  std::uint32_t height_ = 0;
  std::uint32_t reset_flags_ = 0;
};

}  // namespace rat
