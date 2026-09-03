#pragma once

#include "rat/collision.hpp"
#include "rat/game_state.hpp"
#include "rat/map_data.hpp"
#include "rat/map_document.hpp"
#include "rat/player.hpp"
#include "rat/surface_query.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rat {

class Audio;
class GameplayNotifyBus;

inline constexpr int kMaxParallelEvents = 8;
inline constexpr int kMaxParallelCommandsPerFrame = 32;

enum class EventWhyNot {
  Ok,
  WrongPage,
  Conditions,
  Height,
  NotOverlapping,
  OutOfActionRange,
  InputBlocked,
  AlreadyRunning,
  AutorunLock,
  ForegroundBusy,
  ParallelLimit,
  AlreadyInside,
};

[[nodiscard]] const char* event_why_not_name(EventWhyNot reason);

struct EventOverlay {
  TileCoord tile;
  RampDirection facing = RampDirection::South;
  float x = 0.0f;
  float z = 0.0f;
};

struct InterpreterDebug {
  std::string event_id;
  int page_index = -1;
  int command_index = -1;
  int wait_frames = 0;
  int route_index = 0;
  bool waiting_message = false;
  bool parallel = false;
};

class EventRuntime {
 public:
  void load(const RuntimeMap& runtime);
  [[nodiscard]] MapCompileResult load(const MapData& map);
  void set_audio(Audio* audio);  // nullable; not owned
  void set_notify(GameplayNotifyBus* notify);  // nullable; not owned
  void clear();

  // One simulation step. `interact_pressed` is edge-ish: true on the frame interact is pressed.
  void update(GameState& state, const PlayerBody& player, bool interact_pressed, float dt);

  void acknowledge_message();

  [[nodiscard]] bool player_input_blocked() const;
  [[nodiscard]] const std::optional<std::string>& active_message() const { return active_message_; }
  [[nodiscard]] bool has_action_prompt(const PlayerBody& player, const GameState& state) const;
  [[nodiscard]] int active_parallel_count() const { return active_parallel_count_; }
  [[nodiscard]] int last_parallel_commands_executed() const {
    return last_parallel_commands_executed_;
  }
  [[nodiscard]] int last_commands_executed() const { return last_commands_executed_; }
  [[nodiscard]] const MapData& map() const { return runtime_map_.data; }
  [[nodiscard]] const RuntimeMap& runtime_map() const { return runtime_map_; }
  [[nodiscard]] const std::vector<std::string>& warnings() const { return warnings_; }
  [[nodiscard]] std::vector<std::string> overlapping_event_ids(const PlayerBody& player) const;
  [[nodiscard]] std::optional<InterpreterDebug> foreground_debug() const;
  [[nodiscard]] std::vector<InterpreterDebug> parallel_debug() const;
  [[nodiscard]] EventWhyNot why_not_fired(std::string_view event_id, const GameState& state,
                                            const PlayerBody& player, bool interact_pressed) const;
  [[nodiscard]] bool player_overlaps(const EventDef& event, const PlayerBody& player) const;
  [[nodiscard]] std::optional<EventOverlay> event_overlay(std::string_view event_id) const;
  [[nodiscard]] std::vector<Vec3> event_markers() const;

 private:
  struct StackFrame {
    const std::vector<Command>* commands = nullptr;
    std::size_t index = 0;
  };

  struct Interpreter {
    std::string event_id;
    int page_index = -1;
    std::vector<StackFrame> stack;
    int wait_frames = 0;
    int route_index = 0;
    bool route_budget_paid = false;
    bool waiting_message = false;
    bool parallel = false;
    bool autorun = false;
    bool finished = false;
  };

  [[nodiscard]] bool conditions_met(const std::vector<Condition>& conditions,
                                    const GameState& state,
                                    const std::string& event_id) const;
  [[nodiscard]] bool condition_met(const Condition& condition, const GameState& state,
                                   const std::string& event_id) const;
  [[nodiscard]] int select_page(const EventDef& event, const GameState& state) const;
  [[nodiscard]] Aabb2 event_bounds(const EventDef& event) const;
  [[nodiscard]] SurfaceSample event_surface_sample(const EventDef& event) const;
  [[nodiscard]] bool event_height_matches_player(const EventDef& event,
                                                 const PlayerBody& player) const;
  [[nodiscard]] bool action_in_range(const EventDef& event, const PlayerBody& player) const;

  void try_start_autorun(GameState& state);
  void try_start_parallels(GameState& state);
  void try_start_action(GameState& state, const PlayerBody& player, bool interact_pressed);
  void try_start_player_touch(GameState& state, const PlayerBody& player);

  void start_page(const EventDef& event, int page_index, bool parallel, bool autorun);
  void step_interpreter(Interpreter& interp, GameState& state, const PlayerBody& player,
                         int& command_budget);
  bool exec_command(Interpreter& interp, GameState& state, const Command& command);
  [[nodiscard]] InterpreterDebug to_debug(const Interpreter& interp) const;
  [[nodiscard]] const EventDef* find_event(std::string_view event_id) const;
  [[nodiscard]] Vec3 live_event_xz(const EventDef& event) const;
  EventOverlay& ensure_overlay(const EventDef& event);
  [[nodiscard]] bool tile_on_map(TileCoord tile) const;
  [[nodiscard]] bool dest_blocked(TileCoord dest, const PlayerBody& player, bool through,
                                   bool parallel) const;
  bool exec_set_move_route(Interpreter& interp, const Command& command, const PlayerBody& player);

  RuntimeMap runtime_map_;
  Audio* audio_ = nullptr;
  GameplayNotifyBus* notify_ = nullptr;
  std::optional<Interpreter> foreground_;  // autorun / action / touch (blocking)
  std::vector<Interpreter> parallels_;
  std::optional<std::string> active_message_;
  std::unordered_set<std::string> touch_inside_;
  std::unordered_set<std::string> parallel_started_;
  std::unordered_set<std::string> autorun_lock_;
  int active_parallel_count_ = 0;
  int last_parallel_commands_executed_ = 0;
  int last_commands_executed_ = 0;
  std::vector<std::string> warnings_;
  std::unique_ptr<SurfaceQuery> surface_query_;
  CollisionWorld collision_world_{};
  std::unordered_map<std::string, EventOverlay> overlays_;
  float dt_ = 0.0f;
  bool have_last_player_ = false;
  float last_player_x_ = 0.0f;
  float last_player_y_ = 0.0f;
  float last_player_z_ = 0.0f;
};

[[nodiscard]] EventWhyNot event_why_not_fired(const EventRuntime& runtime,
                                               std::string_view event_id, const GameState& state,
                                               const PlayerBody& player, bool interact_pressed);

}  // namespace rat
