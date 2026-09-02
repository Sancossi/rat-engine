#pragma once

#include "rat/player.hpp"

namespace rat {

enum class LocomotionState { Idle, Walk, Jump, Fall, Climb };

[[nodiscard]] const char* locomotion_state_name(LocomotionState state);

// Animation/debug classify from the last integrated JumpState.
// Physics resolves Climb from ladder overlap (see integrate_player_frame_surface).
[[nodiscard]] LocomotionState locomotion_from(const JumpState& jump, const MoveInput& move);

}  // namespace rat
