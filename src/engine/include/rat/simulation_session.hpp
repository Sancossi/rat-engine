#pragma once

#include "rat/app_mode.hpp"
#include "rat/buffered_press.hpp"
#include "rat/clock.hpp"
#include "rat/event_runtime.hpp"
#include "rat/game_state.hpp"
#include "rat/input.hpp"
#include "rat/map_data.hpp"
#include "rat/map_document.hpp"
#include "rat/player.hpp"
#include "rat/save_game.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace rat {

class Audio;
class GameplayNotifyBus;
class SurfaceQuery;

inline constexpr float kSimulationFixedDt = 1.0f / 120.0f;
inline constexpr float kMaxCatchUpSeconds = 0.2f;
inline constexpr int kMaxCatchUpTicks =
    static_cast<int>(kMaxCatchUpSeconds / kSimulationFixedDt + 0.5f);

struct SimulationConfig {
  float dt = kSimulationFixedDt;
  JumpTuning jump_tuning{};
  AppMode app_mode = AppMode::Play;
  float interact_buffer_seconds = 0.1f;
  float max_step_up = 0.35f;
};

struct SimulationTickResult {
  std::uint64_t tick_id = 0;
  bool landed = false;
  bool transferred = false;
};

struct SimulationCatchUpResult {
  int ticks_run = 0;
  bool budget_exceeded = false;
};

struct SimulationLoadResult {
  bool ok = false;
  std::vector<MapIssue> issues;
};

class SimulationSession {
 public:
  SimulationSession() = default;
  explicit SimulationSession(SimulationConfig config);

  [[nodiscard]] SimulationLoadResult load(const MapData& map);
  // Apply a loaded GameState as session progress. Reloads event interpreters from
  // the current map so a prior session.load() cannot wipe switches/vars/items.
  // Fails if loaded.map_id() is non-empty and does not match the current map.
  [[nodiscard]] GameFileResult apply_loaded_game(const GameState& loaded);
  void set_player(PlayerBody player);
  void set_app_mode(AppMode mode);
  void set_notify(GameplayNotifyBus* notify);
  void set_audio(Audio* audio);
  void set_clock(Clock* clock);
  void rebuild_surface();
  void reset_jump_grounded();
  // Capture / reset: drop interact + jump pending and JumpState::jump_buffer_left.
  // Do not call just because this tick's jump_pressed is false.
  void clear_pending_input();
  // Hold a jump edge until the next tick that runs player control. Used when a
  // display frame sees jump_pressed but drain runs zero ticks.
  void note_jump_pressed();
  // Push interact_buffer_ and latch interact_press_pending_ when a display frame
  // sees interact_pressed but drain runs zero ticks. Do not clear the buffer just
  // because this frame has no edge.
  void note_interact_pressed();

  SimulationTickResult tick(const InputFrame& input);

  [[nodiscard]] const PlayerBody& player() const { return player_; }
  [[nodiscard]] PlayerBody& player() { return player_; }
  [[nodiscard]] const JumpState& jump() const { return jump_; }
  [[nodiscard]] JumpState& jump() { return jump_; }
  [[nodiscard]] const GameState& state() const { return state_; }
  [[nodiscard]] GameState& state() { return state_; }
  [[nodiscard]] const EventRuntime& events() const { return events_; }
  [[nodiscard]] EventRuntime& events() { return events_; }
  [[nodiscard]] std::uint64_t tick_id() const { return tick_id_; }
  [[nodiscard]] const SimulationConfig& config() const { return config_; }
  [[nodiscard]] JumpTuning& jump_tuning() { return config_.jump_tuning; }
  [[nodiscard]] const JumpTuning& jump_tuning() const { return config_.jump_tuning; }
  [[nodiscard]] std::unique_ptr<SurfaceQuery>& surface_query() { return surface_; }
  [[nodiscard]] const SurfaceQuery* surface() const { return surface_.get(); }
  [[nodiscard]] double last_tick_seconds() const { return last_tick_seconds_; }

 private:
  void ensure_surface();
  void snap_player_to_ground();

  SimulationConfig config_{};
  PlayerBody player_{};
  JumpState jump_ = make_grounded_jump_state();
  GameState state_{};
  EventRuntime events_{};
  std::uint64_t tick_id_ = 0;
  BufferedPress interact_buffer_{};
  bool jump_press_pending_ = false;
  bool interact_press_pending_ = false;
  std::unique_ptr<SurfaceQuery> surface_;
  GameplayNotifyBus* notify_ = nullptr;
  Clock* clock_ = nullptr;
  SteadyClock steady_{};
  double last_tick_seconds_ = 0.0;
};

// Drain `accumulator` by calling `tick`. Edges on `input` apply to the first tick that
// runs so a reused display-frame InputFrame does not retrigger jump/interact. A jump or
// interact edge on a drain that runs zero ticks is latched via note_jump_pressed() /
// note_interact_pressed() (event buffer and player mount).
[[nodiscard]] SimulationCatchUpResult drain_simulation_catch_up(
    SimulationSession& session, float& accumulator, const InputFrame& input,
    int max_ticks = kMaxCatchUpTicks);

}  // namespace rat

#if defined(HWND) || defined(_WINDOWS_) || defined(GLFW_TRUE) || defined(__glfw3_h__)
#error SimulationSession must not include Win32 HWND or GLFW
#endif
