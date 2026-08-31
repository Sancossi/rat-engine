#pragma once

#include "rat/player.hpp"

namespace rat {

struct InputButtons {
  bool move_up = false;
  bool move_down = false;
  bool move_left = false;
  bool move_right = false;
  bool jump = false;
  bool interact = false;
  bool toggle_mode = false;
  bool hot_apply = false;
  bool cycle_camera = false;
  bool debug_snapshot = false;
};

struct InputGating {
  bool player_control = true;
  bool keyboard_captured = false;
  bool dialog_open = false;
  bool player_input_blocked = false;
};

struct InputFrame {
  MoveInput move{};
  bool jump_pressed = false;
  bool jump_held = false;
  bool interact_pressed = false;
  bool toggle_mode_pressed = false;
  bool hot_apply_pressed = false;
  bool cycle_camera_pressed = false;
  bool debug_snapshot_pressed = false;
};

[[nodiscard]] InputFrame map_input_frame(const InputButtons& down, const InputButtons& previous,
                                         const InputGating& gating);

[[nodiscard]] PlayerFrameInput player_input_from_frame(const InputFrame& frame);

}  // namespace rat
