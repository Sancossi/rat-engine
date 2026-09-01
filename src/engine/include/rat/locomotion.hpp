#pragma once

#include "rat/player.hpp"

namespace rat {

enum class LocomotionState { Idle, Walk, Jump, Fall };

[[nodiscard]] const char* locomotion_state_name(LocomotionState state);

// Pure classify. Does not mutate JumpState or integrate.
[[nodiscard]] LocomotionState locomotion_from(const JumpState& jump, const MoveInput& move);

}  // namespace rat
