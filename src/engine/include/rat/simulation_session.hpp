#pragma once

#include "rat/app_mode.hpp"
#include "rat/buffered_press.hpp"
#include "rat/event_runtime.hpp"
#include "rat/game_state.hpp"
#include "rat/input.hpp"
#include "rat/map_data.hpp"
#include "rat/map_document.hpp"
#include "rat/player.hpp"

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

  SimulationLoadResult load(const MapData& map);
  void set_player(PlayerBody player);
  void set_app_mode(AppMode mode);
  void set_notify(GameplayNotifyBus* notify);
  void set_audio(Audio* audio);
  void rebuild_surface();
  void reset_jump_grounded();
  // Capture / reset: drop interact + jump pending and JumpState::jump_buffer_left.
  // Do not call just because this tick's jump_pressed is false.
  void clear_pending_input();

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
  std::unique_ptr<SurfaceQuery> surface_;
  GameplayNotifyBus* notify_ = nullptr;
};

// Drain `accumulator` by calling `tick`. Edges on `input` apply to the first tick only so a
// reused display-frame InputFrame does not retrigger jump/interact on catch-up ticks.
[[nodiscard]] SimulationCatchUpResult drain_simulation_catch_up(
    SimulationSession& session, float& accumulator, const InputFrame& input,
    int max_ticks = kMaxCatchUpTicks);

}  // namespace rat
