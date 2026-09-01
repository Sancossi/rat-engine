#include "rat/input.hpp"

namespace rat {

namespace {

[[nodiscard]] bool edge(bool down, bool previous) {
  return down && !previous;
}

}  // namespace

InputFrame map_input_frame(const InputButtons& down, const InputButtons& previous,
                           const InputGating& gating) {
  InputFrame frame;
  const bool allow_player =
      gating.player_control && !gating.keyboard_captured && !gating.player_input_blocked;
  if (allow_player) {
    float screen_x = 0.0f;
    float screen_z = 0.0f;
    if (down.move_up) {
      screen_z += 1.0f;
    }
    if (down.move_down) {
      screen_z -= 1.0f;
    }
    if (down.move_left) {
      screen_x -= 1.0f;
    }
    if (down.move_right) {
      screen_x += 1.0f;
    }
    frame.move = world_aligned_move(screen_x, screen_z);
    frame.jump_pressed = edge(down.jump, previous.jump);
    frame.jump_held = down.jump;
  }

  frame.interact_pressed = edge(down.interact, previous.interact) && !gating.keyboard_captured;
  frame.toggle_mode_pressed = edge(down.toggle_mode, previous.toggle_mode) && !gating.dialog_open;
  frame.hot_apply_pressed = edge(down.hot_apply, previous.hot_apply) && !gating.dialog_open;
  frame.cycle_camera_pressed =
      edge(down.cycle_camera, previous.cycle_camera) && !gating.keyboard_captured &&
      !gating.dialog_open;
  frame.debug_snapshot_pressed =
      edge(down.debug_snapshot, previous.debug_snapshot) && !gating.keyboard_captured;
  frame.undo_pressed = edge(down.undo, previous.undo) && !gating.keyboard_captured;
  frame.redo_pressed = edge(down.redo, previous.redo) && !gating.keyboard_captured;
  return frame;
}

PlayerFrameInput player_input_from_frame(const InputFrame& frame) {
  PlayerFrameInput input;
  input.move = frame.move;
  input.jump_pressed = frame.jump_pressed;
  input.jump_held = frame.jump_held;
  return input;
}

}  // namespace rat
