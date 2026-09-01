#pragma once

#include <functional>

namespace rat {

struct FrameCoordinator {
  std::function<void()> poll;
  std::function<void(float dt)> simulate;
  std::function<void()> drain_audio;
  std::function<void()> begin_ui;
  std::function<void()> draw_ui;
  std::function<void()> present;

  void run_frame(float dt) const;
};

}  // namespace rat
