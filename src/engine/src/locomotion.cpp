#include "rat/locomotion.hpp"

#include <cmath>

namespace rat {

const char* locomotion_state_name(LocomotionState state) {
  switch (state) {
    case LocomotionState::Idle:
      return "Idle";
    case LocomotionState::Walk:
      return "Walk";
    case LocomotionState::Jump:
      return "Jump";
    case LocomotionState::Fall:
      return "Fall";
    case LocomotionState::Climb:
      return "Climb";
  }
  return "Idle";
}

LocomotionState locomotion_from(const JumpState& jump, const MoveInput& move) {
  if (jump.climbing) {
    return LocomotionState::Climb;
  }
  if (!jump.grounded) {
    if (jump.vertical_speed > 0.0f) {
      return LocomotionState::Jump;
    }
    return LocomotionState::Fall;
  }
  const float length = std::hypot(move.axis_x, move.axis_z);
  if (length > 1e-4f) {
    return LocomotionState::Walk;
  }
  return LocomotionState::Idle;
}

}  // namespace rat
