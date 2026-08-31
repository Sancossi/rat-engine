#pragma once

#include "rat/game_state.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rat {

inline constexpr int kMaxParallelEvents = 8;
inline constexpr int kMaxParallelCommandsPerFrame = 32;

class EventRuntime {
 public:
  void load(MapData map);
  void set_blockers(std::vector<Aabb2> blockers);
  void set_events(std::vector<EventDef> events);
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
  [[nodiscard]] const MapData& map() const { return map_; }
  [[nodiscard]] const std::vector<std::string>& warnings() const { return warnings_; }

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
  [[nodiscard]] bool player_overlaps(const EventDef& event, const PlayerBody& player) const;
  [[nodiscard]] bool action_in_range(const EventDef& event, const PlayerBody& player) const;

  void try_start_autorun(GameState& state);
  void try_start_parallels(GameState& state);
  void try_start_action(GameState& state, const PlayerBody& player, bool interact_pressed);
  void try_start_player_touch(GameState& state, const PlayerBody& player);

  void start_page(const EventDef& event, int page_index, bool parallel, bool autorun);
  void step_interpreter(Interpreter& interp, GameState& state, int& command_budget);
  bool exec_command(Interpreter& interp, GameState& state, const Command& command);

  MapData map_;
  std::optional<Interpreter> foreground_;  // autorun / action / touch (blocking)
  std::vector<Interpreter> parallels_;
  std::optional<std::string> active_message_;
  std::unordered_set<std::string> touch_inside_;
  std::unordered_set<std::string> parallel_started_;
  std::unordered_set<std::string> autorun_lock_;
  int active_parallel_count_ = 0;
  int last_parallel_commands_executed_ = 0;
  std::vector<std::string> warnings_;
};

}  // namespace rat
