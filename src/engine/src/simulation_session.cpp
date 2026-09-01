#include "rat/simulation_session.hpp"

#include "rat/gameplay_notify.hpp"
#include "rat/map_document.hpp"
#include "rat/surface_query.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace rat {

SimulationSession::SimulationSession(SimulationConfig config) : config_(std::move(config)) {
  jump_ = make_grounded_jump_state();
  jump_.coyote_time_left = config_.jump_tuning.coyote_seconds;
  jump_.jump_buffer_left = 0.0f;
}

SimulationLoadResult SimulationSession::load(const MapData& map) {
  SimulationLoadResult result;
  const MapCompileResult compiled = compile_map_data(map);
  result.issues = compiled.issues;
  if (!compiled.ok) {
    result.ok = false;
    return result;
  }

  events_.load(compiled.runtime);
  state_.set_map_id(compiled.runtime.data.id);
  state_.set_player_position(player_.x, player_.y, player_.z);
  jump_ = make_grounded_jump_state();
  jump_.coyote_time_left = config_.jump_tuning.coyote_seconds;
  jump_.jump_buffer_left = 0.0f;
  tick_id_ = 0;
  clear_pending_input();
  rebuild_surface();
  result.ok = true;
  return result;
}

void SimulationSession::set_player(PlayerBody player) {
  player_ = player;
  state_.set_player_position(player_.x, player_.y, player_.z);
}

void SimulationSession::set_app_mode(AppMode mode) {
  config_.app_mode = mode;
  if (mode != AppMode::Play) {
    clear_buffered_press(interact_buffer_);
  }
}

void SimulationSession::set_notify(GameplayNotifyBus* notify) {
  notify_ = notify;
  events_.set_notify(notify);
}

void SimulationSession::set_audio(Audio* audio) {
  events_.set_audio(audio);
}

void SimulationSession::rebuild_surface() {
  surface_ = std::make_unique<SurfaceQuery>(events_.map());
}

void SimulationSession::ensure_surface() {
  if (surface_ == nullptr) {
    rebuild_surface();
  }
}

void SimulationSession::snap_player_to_ground() {
  ensure_surface();
  player_.y = surface_->sample(player_.x, player_.z).y;
  state_.set_player_position(player_.x, player_.y, player_.z);
}

void SimulationSession::reset_jump_grounded() {
  jump_ = make_grounded_jump_state();
  jump_.grounded = true;
  jump_.jump_offset = 0.0f;
  jump_.vertical_speed = 0.0f;
  jump_.coyote_time_left = config_.jump_tuning.coyote_seconds;
  jump_.jump_buffer_left = 0.0f;
  jump_press_pending_ = false;
  snap_player_to_ground();
}

void SimulationSession::clear_pending_input() {
  jump_press_pending_ = false;
  jump_.jump_buffer_left = 0.0f;
  clear_buffered_press(interact_buffer_);
}

void SimulationSession::note_jump_pressed() {
  const bool control = player_control_enabled(config_.app_mode);
  const bool blocked = events_.player_input_blocked();
  if (control && !blocked) {
    jump_press_pending_ = true;
  }
}

void SimulationSession::note_interact_pressed() {
  push_buffered_press_if_allowed(interact_buffer_, true, event_runtime_enabled(config_.app_mode),
                                 false, config_.interact_buffer_seconds);
}

SimulationTickResult SimulationSession::tick(const InputFrame& input) {
  SimulationTickResult result;
  ++tick_id_;
  result.tick_id = tick_id_;

  const float dt = config_.dt;
  ensure_surface();

  const bool control = player_control_enabled(config_.app_mode);
  const bool events_on = event_runtime_enabled(config_.app_mode);
  const bool blocked = events_.player_input_blocked();
  const bool allow_player_input = control && !blocked;

  if (input.jump_pressed && allow_player_input) {
    jump_press_pending_ = true;
  }

  push_buffered_press_if_allowed(interact_buffer_, input.interact_pressed, events_on, false,
                                 config_.interact_buffer_seconds);

  if (!allow_player_input) {
    jump_.jump_buffer_left = 0.0f;
    jump_press_pending_ = false;
  }

  if (control) {
    PlayerFrameInput frame_input = player_input_from_frame(input);
    if (!allow_player_input) {
      frame_input.move = {};
      frame_input.jump_pressed = false;
      frame_input.jump_held = false;
    } else {
      frame_input.jump_pressed = jump_press_pending_;
      frame_input.jump_held = input.jump_held;
    }

    const PlayerFrameResult integrated = integrate_player_frame_surface(
        player_, jump_, frame_input, dt, events_.map().blockers, *surface_, config_.jump_tuning,
        config_.max_step_up, events_.map().edge_barriers, &events_.map());
    player_ = integrated.body;
    jump_ = integrated.jump;
    result.landed = integrated.landed;
    if (integrated.landed && notify_ != nullptr) {
      notify_->post({GameplayNotifyKind::Landed, {}});
    }
    state_.set_player_position(player_.x, player_.y, player_.z);
    if (frame_input.jump_pressed) {
      jump_press_pending_ = false;
    }
  } else {
    reset_jump_grounded();
  }

  if (events_on) {
    bool consumed_for_dialog = false;
    if (events_.active_message().has_value() && consume_buffered_press(interact_buffer_)) {
      events_.acknowledge_message();
      consumed_for_dialog = true;
    }

    const bool gameplay_interact =
        !consumed_for_dialog && !events_.player_input_blocked() &&
        consume_buffered_press(interact_buffer_);

    const std::string before_map_id = state_.map_id();
    const float before_x = state_.player_x();
    const float before_y = state_.player_y();
    const float before_z = state_.player_z();
    events_.update(state_, player_, gameplay_interact, dt);

    const bool transferred = state_.map_id() != before_map_id || state_.player_x() != before_x ||
                             state_.player_y() != before_y || state_.player_z() != before_z;
    player_.x = state_.player_x();
    player_.y = state_.player_y();
    player_.z = state_.player_z();
    result.transferred = transferred;
    if (transferred) {
      jump_ = make_grounded_jump_state();
      jump_.coyote_time_left = config_.jump_tuning.coyote_seconds;
      jump_.jump_buffer_left = 0.0f;
      jump_press_pending_ = false;
    }
  }

  consume_then_tick_buffered_press(interact_buffer_, false, dt);
  return result;
}

SimulationCatchUpResult drain_simulation_catch_up(SimulationSession& session, float& accumulator,
                                                  const InputFrame& input, int max_ticks) {
  SimulationCatchUpResult result;
  const float dt = session.config().dt;
  if (dt <= 0.0f || max_ticks <= 0) {
    return result;
  }

  const int available = static_cast<int>(accumulator / dt);
  result.budget_exceeded = available > max_ticks;
  const int to_run = std::min(available, max_ticks);

  InputFrame tick_input = input;
  if (input.jump_pressed) {
    session.note_jump_pressed();
  }
  if (input.interact_pressed) {
    session.note_interact_pressed();
  }
  for (int i = 0; i < to_run; ++i) {
    accumulator -= dt;
    session.tick(tick_input);
    tick_input.jump_pressed = false;
    tick_input.interact_pressed = false;
    ++result.ticks_run;
  }

  if (result.budget_exceeded) {
    while (accumulator >= dt) {
      accumulator -= dt;
    }
  }

  return result;
}

}  // namespace rat
