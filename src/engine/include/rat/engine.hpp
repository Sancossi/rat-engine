#pragma once

#include "rat/greybox.hpp"
#include "rat/renderer.hpp"

#include <string>

namespace rat {

class Engine {
 public:
  Engine() = default;
  ~Engine();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  bool init(const RendererConfig& config);
  void shutdown();
  void resize(std::uint32_t width, std::uint32_t height);

  // begin_frame: clear + greybox + dbgText. end_frame: bgfx::frame().
  // Call ImGui (or other views) between begin_frame and end_frame.
  void begin_frame();
  void end_frame();
  void frame();

  [[nodiscard]] bool is_initialized() const { return initialized_; }
  [[nodiscard]] const GreyboxScene& greybox() const { return greybox_; }

  void set_debug_banner(std::string banner) { debug_banner_ = std::move(banner); }

 private:
  Renderer renderer_;
  GreyboxScene greybox_;
  bool initialized_ = false;
  std::string debug_banner_{"rat-engine"};
};

}  // namespace rat
