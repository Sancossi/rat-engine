#include "frame_coordinator.hpp"

namespace rat {

void FrameCoordinator::run_frame(float dt) const {
  if (poll) {
    poll();
  }
  if (simulate) {
    simulate(dt);
  }
  if (drain_audio) {
    drain_audio();
  }
  if (begin_ui) {
    begin_ui();
  }
  if (draw_ui) {
    draw_ui();
  }
  if (present) {
    present();
  }
}

}  // namespace rat
