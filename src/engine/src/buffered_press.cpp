#include "rat/buffered_press.hpp"

#include <algorithm>

namespace rat {

void push_buffered_press(BufferedPress& press, float buffer_seconds) {
  if (buffer_seconds <= 0.0f) {
    return;
  }
  press.seconds_left = std::max(press.seconds_left, buffer_seconds);
}

void tick_buffered_press(BufferedPress& press, float dt) {
  if (dt <= 0.0f || press.seconds_left <= 0.0f) {
    return;
  }
  press.seconds_left = std::max(0.0f, press.seconds_left - dt);
}

bool consume_buffered_press(BufferedPress& press) {
  if (press.seconds_left <= 0.0f) {
    return false;
  }
  press.seconds_left = 0.0f;
  return true;
}

bool consume_then_tick_buffered_press(BufferedPress& press, bool consume_allowed, float dt) {
  const bool consumed = consume_allowed ? consume_buffered_press(press) : false;
  tick_buffered_press(press, dt);
  return consumed;
}

void clear_buffered_press(BufferedPress& press) {
  press.seconds_left = 0.0f;
}

bool has_buffered_press(const BufferedPress& press) {
  return press.seconds_left > 0.0f;
}

void push_buffered_press_if_allowed(BufferedPress& press, bool pressed_edge, bool runtime_enabled,
                                    bool keyboard_captured, float buffer_seconds) {
  if (!pressed_edge || !runtime_enabled || keyboard_captured) {
    return;
  }
  push_buffered_press(press, buffer_seconds);
}

}  // namespace rat
