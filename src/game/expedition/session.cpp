#include "session.hpp"
#include <cmath>
#include <stdexcept>
namespace rat::expedition {
ExpeditionSession::ExpeditionSession(Project project) : project_(std::move(project)), simulation_(std::make_unique<SimulationSession>()) {
  const auto result = simulation_->load(scene().map);
  if (!result.ok) throw std::runtime_error("Cannot compile scene " + scene().id);
  PlayerBody body;
  const auto spawn = scene().spawns.at("entry");
  body.x = spawn.x; body.y = spawn.y; body.z = spawn.z; body.speed = 3; body.half_extent = 0.2f;
  simulation_->reset_jump_grounded();
  simulation_->set_player(body);
}
void ExpeditionSession::tick(const TraversalInput& input) {
  InputButtons down;
  down.move_right = input.screen_x > 0; down.move_left = input.screen_x < 0;
  down.move_down = input.screen_y > 0; down.move_up = input.screen_y < 0;
  const auto focus = scene().camera_focus;
  const Vec3 eye{focus.x + 16, focus.y + 20, focus.z + 16};
  auto frame = map_input_frame(down, {}, {}, eye, focus);
  // P1.1 deliberately has no jump, stance, or interaction action.
  simulation_->tick(frame);
  moving_ = input.screen_x != 0 || input.screen_y != 0;
  if (moving_) direction_ = std::abs(input.screen_x) > std::abs(input.screen_y) ? (input.screen_x > 0 ? 2 : 1) : (input.screen_y > 0 ? 0 : 3);
}
TraversalSnapshot ExpeditionSession::snapshot() const {
  const auto& p = simulation_->player();
  return {scene().id, {p.x, p.y, p.z}, simulation_->tick_id(), direction_, moving_ ? static_cast<int>((simulation_->tick_id() / 18) % 2) : 0};
}
}
