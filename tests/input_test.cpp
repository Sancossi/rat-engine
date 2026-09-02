#include <rat/app_mode.hpp>
#include <rat/input.hpp>
#include <rat/player.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("InputFrame default camera walk matches world-aligned fallback", "[unit][input]") {
  rat::InputButtons down;
  down.move_up = true;
  down.move_right = true;
  rat::InputGating gate;

  const rat::InputFrame frame = rat::map_input_frame(down, {}, gate);
  const rat::MoveInput expected = rat::world_aligned_move(1.0f, 1.0f);
  CHECK(frame.move.axis_x == expected.axis_x);
  CHECK(frame.move.axis_z == expected.axis_z);
  CHECK(frame.climb_move.axis_x == frame.move.axis_x);
  CHECK(frame.climb_move.axis_z == frame.move.axis_z);
}

TEST_CASE("InputFrame walk follows camera look", "[unit][input]") {
  rat::InputButtons down;
  down.move_up = true;
  rat::InputGating gate;
  const rat::Vec3 eye{0.0f, 8.0f, 0.0f};
  const rat::Vec3 focus{4.0f, 0.0f, 0.0f};

  const rat::InputFrame frame = rat::map_input_frame(down, {}, gate, eye, focus);
  const rat::MoveInput cam_w = rat::camera_relative_move(0.0f, 1.0f, eye, focus);
  CHECK(frame.move.axis_x == cam_w.axis_x);
  CHECK(frame.move.axis_z == cam_w.axis_z);
  CHECK(frame.climb_move.axis_x == frame.move.axis_x);
  CHECK(frame.climb_move.axis_z == frame.move.axis_z);
  CHECK(frame.move.axis_x > 0.5f);
}

TEST_CASE("InputFrame reports jump, interact, and editor action edges", "[unit][input]") {
  rat::InputButtons down;
  down.jump = true;
  down.interact = true;
  down.toggle_mode = true;
  down.hot_apply = true;
  down.cycle_camera = true;

  const rat::InputFrame pressed = rat::map_input_frame(down, {}, {});
  CHECK(pressed.jump_pressed);
  CHECK(pressed.jump_held);
  CHECK(pressed.interact_pressed);
  CHECK(pressed.toggle_mode_pressed);
  CHECK(pressed.hot_apply_pressed);
  CHECK(pressed.cycle_camera_pressed);

  const rat::InputFrame held = rat::map_input_frame(down, down, {});
  CHECK_FALSE(held.jump_pressed);
  CHECK(held.jump_held);
  CHECK_FALSE(held.interact_pressed);
  CHECK_FALSE(held.toggle_mode_pressed);
  CHECK_FALSE(held.hot_apply_pressed);
  CHECK_FALSE(held.cycle_camera_pressed);
}

TEST_CASE("InputFrame Play/Edit gating matches current editor rules", "[unit][input]") {
  rat::InputButtons down;
  down.move_up = true;
  down.jump = true;
  down.interact = true;
  down.toggle_mode = true;
  down.hot_apply = true;
  down.cycle_camera = true;

  rat::InputGating edit;
  edit.player_control = rat::player_control_enabled(rat::AppMode::Edit);
  const rat::InputFrame edit_frame = rat::map_input_frame(down, {}, edit);
  CHECK(edit_frame.move.axis_x == 0.0f);
  CHECK(edit_frame.move.axis_z == 0.0f);
  CHECK_FALSE(edit_frame.jump_pressed);
  CHECK_FALSE(edit_frame.jump_held);
  CHECK(edit_frame.toggle_mode_pressed);
  CHECK(edit_frame.hot_apply_pressed);
  CHECK(edit_frame.cycle_camera_pressed);
  CHECK(edit_frame.interact_pressed);

  rat::InputGating captured;
  captured.keyboard_captured = true;
  const rat::InputFrame captured_frame = rat::map_input_frame(down, {}, captured);
  CHECK(captured_frame.move.axis_x == 0.0f);
  CHECK_FALSE(captured_frame.jump_pressed);
  CHECK_FALSE(captured_frame.interact_pressed);
  CHECK_FALSE(captured_frame.cycle_camera_pressed);
  CHECK(captured_frame.toggle_mode_pressed);
  CHECK(captured_frame.hot_apply_pressed);

  rat::InputGating dialog;
  dialog.dialog_open = true;
  const rat::InputFrame dialog_frame = rat::map_input_frame(down, {}, dialog);
  CHECK_FALSE(dialog_frame.toggle_mode_pressed);
  CHECK_FALSE(dialog_frame.hot_apply_pressed);
  CHECK_FALSE(dialog_frame.cycle_camera_pressed);

  rat::InputGating blocked;
  blocked.player_input_blocked = true;
  const rat::InputFrame blocked_frame = rat::map_input_frame(down, {}, blocked);
  CHECK(blocked_frame.move.axis_x == 0.0f);
  CHECK_FALSE(blocked_frame.jump_pressed);
  CHECK_FALSE(blocked_frame.jump_held);
}

TEST_CASE("player_input_from_frame copies move and jump", "[unit][input]") {
  rat::InputFrame frame;
  frame.move = rat::MoveInput{0.5f, -1.0f};
  frame.climb_move = rat::MoveInput{0.25f, 0.75f};
  frame.jump_pressed = true;
  frame.jump_held = true;
  frame.interact_pressed = true;

  const rat::PlayerFrameInput player = rat::player_input_from_frame(frame);
  CHECK(player.move.axis_x == 0.5f);
  CHECK(player.move.axis_z == -1.0f);
  CHECK(player.climb_move.axis_x == 0.25f);
  CHECK(player.climb_move.axis_z == 0.75f);
  CHECK(player.jump_pressed);
  CHECK(player.jump_held);
  CHECK(player.interact_pressed);
}

TEST_CASE("InputFrame reports undo and redo edges unless keyboard captured", "[unit][input]") {
  rat::InputButtons down;
  down.undo = true;
  down.redo = true;

  const rat::InputFrame pressed = rat::map_input_frame(down, {}, {});
  CHECK(pressed.undo_pressed);
  CHECK(pressed.redo_pressed);

  const rat::InputFrame held = rat::map_input_frame(down, down, {});
  CHECK_FALSE(held.undo_pressed);
  CHECK_FALSE(held.redo_pressed);

  rat::InputGating captured;
  captured.keyboard_captured = true;
  const rat::InputFrame captured_frame = rat::map_input_frame(down, {}, captured);
  CHECK_FALSE(captured_frame.undo_pressed);
  CHECK_FALSE(captured_frame.redo_pressed);
}
