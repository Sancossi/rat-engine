#pragma once

namespace rat {

struct BufferedPress {
  float seconds_left = 0.0f;
};

void push_buffered_press(BufferedPress& press, float buffer_seconds);
void tick_buffered_press(BufferedPress& press, float dt);
bool consume_buffered_press(BufferedPress& press);
bool consume_then_tick_buffered_press(BufferedPress& press, bool consume_allowed, float dt);
void clear_buffered_press_if_captured(BufferedPress& press, bool keyboard_captured);
void clear_buffered_press(BufferedPress& press);
[[nodiscard]] bool has_buffered_press(const BufferedPress& press);
void push_buffered_press_if_allowed(BufferedPress& press, bool pressed_edge, bool runtime_enabled,
                                    bool keyboard_captured, float buffer_seconds);

}  // namespace rat
