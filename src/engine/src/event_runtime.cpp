#include "rat/event_runtime.hpp"

#include "rat/event_edit.hpp"

#include <algorithm>
#include <cmath>

namespace rat {
namespace {

bool compare(int left, CompareOp op, int right) {
  switch (op) {
    case CompareOp::Eq:
      return left == right;
    case CompareOp::Ne:
      return left != right;
    case CompareOp::Lt:
      return left < right;
    case CompareOp::Le:
      return left <= right;
    case CompareOp::Gt:
      return left > right;
    case CompareOp::Ge:
      return left >= right;
  }
  return false;
}

}  // namespace

void EventRuntime::load(MapData map) {
  clear();
  map_ = std::move(map);
}

void EventRuntime::set_blockers(std::vector<Aabb2> blockers) {
  map_.blockers = std::move(blockers);
}

void EventRuntime::set_events(std::vector<EventDef> events) {
  map_.events = std::move(events);
  // Drop live interpreters — page pointers / overlaps may be stale after moves.
  foreground_.reset();
  parallels_.clear();
  active_message_.reset();
  touch_inside_.clear();
  parallel_started_.clear();
  autorun_lock_.clear();
  active_parallel_count_ = 0;
  last_parallel_commands_executed_ = 0;
  warnings_.clear();
}

void EventRuntime::clear() {
  map_ = {};
  foreground_.reset();
  parallels_.clear();
  active_message_.reset();
  touch_inside_.clear();
  parallel_started_.clear();
  autorun_lock_.clear();
  active_parallel_count_ = 0;
  last_parallel_commands_executed_ = 0;
  warnings_.clear();
}

bool EventRuntime::player_input_blocked() const {
  return foreground_.has_value() || active_message_.has_value();
}

bool EventRuntime::has_action_prompt(const PlayerBody& player, const GameState& state) const {
  if (player_input_blocked()) {
    return false;
  }
  for (const EventDef& event : map_.events) {
    if (!action_in_range(event, player)) {
      continue;
    }
    const int page_index = select_page(event, state);
    if (page_index < 0) {
      continue;
    }
    if (event.pages[static_cast<std::size_t>(page_index)].trigger == TriggerKind::Action) {
      return true;
    }
  }
  return false;
}

void EventRuntime::acknowledge_message() {
  if (!active_message_.has_value()) {
    return;
  }
  active_message_.reset();
  if (foreground_.has_value()) {
    foreground_->waiting_message = false;
  }
  for (auto& interp : parallels_) {
    if (interp.waiting_message) {
      interp.waiting_message = false;
    }
  }
}

bool EventRuntime::condition_met(const Condition& condition, const GameState& state,
                                 const std::string& event_id) const {
  switch (condition.type) {
    case ConditionType::Switch:
      return state.get_switch(condition.id) == condition.bool_value;
    case ConditionType::Variable:
      return compare(state.get_variable(condition.id), condition.op, condition.int_value);
    case ConditionType::Item:
      return state.has_item(condition.string_id, condition.int_value);
    case ConditionType::SelfSwitch:
      return state.get_self_switch(event_id, condition.self_switch) == condition.bool_value;
  }
  return false;
}

bool EventRuntime::conditions_met(const std::vector<Condition>& conditions,
                                  const GameState& state,
                                  const std::string& event_id) const {
  for (const Condition& condition : conditions) {
    if (!condition_met(condition, state, event_id)) {
      return false;
    }
  }
  return true;
}

int EventRuntime::select_page(const EventDef& event, const GameState& state) const {
  // RM-like: highest index whose conditions match.
  for (int i = static_cast<int>(event.pages.size()) - 1; i >= 0; --i) {
    if (conditions_met(event.pages[static_cast<std::size_t>(i)].conditions, state, event.id)) {
      return i;
    }
  }
  return -1;
}

Aabb2 EventRuntime::event_bounds(const EventDef& event) const {
  if (event.volume.has_value()) {
    return *event.volume;
  }
  if (event.tile.has_value()) {
    const Vec3 center = tile_center_world(*event.tile, map_.tile_size);
    const float h = 0.5f * map_.tile_size;
    return Aabb2{center.x - h, center.z - h, center.x + h, center.z + h};
  }
  return Aabb2{0, 0, 0, 0};
}

bool EventRuntime::player_overlaps(const EventDef& event, const PlayerBody& player) const {
  if (!event.volume.has_value() && !event.tile.has_value()) {
    return false;
  }
  const Aabb2 box = event_bounds(event);
  const Aabb2 body{player.x - player.half_extent, player.z - player.half_extent,
                   player.x + player.half_extent, player.z + player.half_extent};
  return aabb_overlap(box, body);
}

bool EventRuntime::action_in_range(const EventDef& event, const PlayerBody& player) const {
  if (event.tile.has_value()) {
    const Vec3 center = tile_center_world(*event.tile, map_.tile_size);
    const float dx = player.x - center.x;
    const float dz = player.z - center.z;
    constexpr float kActionRadiusInTiles = 0.65f;
    const float radius = kActionRadiusInTiles * map_.tile_size;
    return dx * dx + dz * dz <= radius * radius;
  }
  // Explicit volumes keep authored AABB semantics.
  return event.volume.has_value() && player_overlaps(event, player);
}

void EventRuntime::start_page(const EventDef& event, int page_index, bool parallel, bool autorun) {
  const EventPage& page = event.pages[static_cast<std::size_t>(page_index)];
  Interpreter interp;
  interp.event_id = event.id;
  interp.page_index = page_index;
  interp.stack.push_back(StackFrame{&page.commands, 0});
  interp.parallel = parallel;
  interp.autorun = autorun;
  if (parallel) {
    parallels_.push_back(std::move(interp));
  } else {
    foreground_ = std::move(interp);
  }
}

void EventRuntime::try_start_autorun(GameState& state) {
  if (foreground_.has_value() || active_message_.has_value()) {
    return;
  }
  for (const EventDef& event : map_.events) {
    if (autorun_lock_.contains(event.id)) {
      continue;
    }
    const int page_index = select_page(event, state);
    if (page_index < 0) {
      continue;
    }
    if (event.pages[static_cast<std::size_t>(page_index)].trigger != TriggerKind::Autorun) {
      continue;
    }
    start_page(event, page_index, false, true);
    autorun_lock_.insert(event.id);
    return;  // only one autorun at a time
  }
}

void EventRuntime::try_start_parallels(GameState& state) {
  for (const EventDef& event : map_.events) {
    if (parallel_started_.contains(event.id)) {
      continue;
    }
    const int page_index = select_page(event, state);
    if (page_index < 0) {
      continue;
    }
    if (event.pages[static_cast<std::size_t>(page_index)].trigger != TriggerKind::Parallel) {
      continue;
    }
    if (static_cast<int>(parallels_.size()) >= kMaxParallelEvents) {
      warnings_.push_back("Parallel limit reached (max 8); skipped event " + event.id);
      continue;
    }
    start_page(event, page_index, true, false);
    parallel_started_.insert(event.id);
  }
}

void EventRuntime::try_start_action(GameState& state, const PlayerBody& player,
                                    bool interact_pressed) {
  if (!interact_pressed || foreground_.has_value() || active_message_.has_value()) {
    return;
  }
  for (const EventDef& event : map_.events) {
    if (!action_in_range(event, player)) {
      continue;
    }
    const int page_index = select_page(event, state);
    if (page_index < 0) {
      continue;
    }
    if (event.pages[static_cast<std::size_t>(page_index)].trigger != TriggerKind::Action) {
      continue;
    }
    start_page(event, page_index, false, false);
    return;
  }
}

void EventRuntime::try_start_player_touch(GameState& state, const PlayerBody& player) {
  if (foreground_.has_value() || active_message_.has_value()) {
    return;
  }
  for (const EventDef& event : map_.events) {
    const bool inside = player_overlaps(event, player);
    const bool was_inside = touch_inside_.contains(event.id);
    if (inside) {
      touch_inside_.insert(event.id);
    } else {
      touch_inside_.erase(event.id);
    }
    if (!(inside && !was_inside)) {
      continue;
    }
    const int page_index = select_page(event, state);
    if (page_index < 0) {
      continue;
    }
    if (event.pages[static_cast<std::size_t>(page_index)].trigger != TriggerKind::PlayerTouch) {
      continue;
    }
    start_page(event, page_index, false, false);
    return;
  }
}

bool EventRuntime::exec_command(Interpreter& interp, GameState& state, const Command& command) {
  switch (command.op) {
    case CommandOp::ShowText:
      active_message_ = command.text;
      interp.waiting_message = true;
      return false;
    case CommandOp::ControlSwitch:
      state.set_switch(command.id, command.bool_value);
      return true;
    case CommandOp::ControlVariable:
      state.set_variable(command.id, command.int_value);
      return true;
    case CommandOp::ControlSelfSwitch:
      state.set_self_switch(interp.event_id, command.self_switch, command.bool_value);
      return true;
    case CommandOp::ConditionalBranch: {
      const bool ok = condition_met(command.branch_condition, state, interp.event_id);
      const std::vector<Command>& branch = ok ? command.then_commands : command.else_commands;
      interp.stack.push_back(StackFrame{&branch, 0});
      return true;
    }
    case CommandOp::Wait:
      interp.wait_frames = command.frames;
      return true;
    case CommandOp::TransferPlayer:
      state.set_map_id(command.map_id);
      state.set_player_position(command.x, command.y, command.z);
      return true;
    case CommandOp::ChangeItems:
      state.add_item(command.item_id, command.item_delta, command.key_item);
      return true;
    case CommandOp::Comment:
      return true;
  }
  return true;
}

void EventRuntime::step_interpreter(Interpreter& interp, GameState& state, int& command_budget) {
  if (interp.finished) {
    return;
  }
  if (interp.waiting_message) {
    return;
  }
  if (interp.wait_frames > 0) {
    --interp.wait_frames;
    return;
  }

  while (!interp.stack.empty()) {
    if (interp.parallel && command_budget <= 0) {
      return;
    }

    StackFrame& frame = interp.stack.back();
    if (frame.commands == nullptr || frame.index >= frame.commands->size()) {
      interp.stack.pop_back();
      continue;
    }

    const Command& command = (*frame.commands)[frame.index];
    ++frame.index;
    if (interp.parallel) {
      --command_budget;
      ++last_parallel_commands_executed_;
    }

    // Nested Parallel is not expressible as a command in v1; keep guard for future ops.
    if (interp.parallel && command.op == CommandOp::Comment && command.text == "__nested_parallel__") {
      warnings_.push_back("Nested Parallel forbidden; ignored");
      continue;
    }

    const bool continue_now = exec_command(interp, state, command);
    if (!continue_now) {
      return;
    }
    if (interp.wait_frames > 0) {
      return;
    }
  }

  interp.finished = true;
}

void EventRuntime::update(GameState& state, const PlayerBody& player, bool interact_pressed,
                          float /*dt*/) {
  last_parallel_commands_executed_ = 0;

  try_start_autorun(state);
  try_start_parallels(state);

  if (!player_input_blocked()) {
    try_start_action(state, player, interact_pressed);
    try_start_player_touch(state, player);
  }

  if (foreground_.has_value()) {
    int unlimited = 100000;
    step_interpreter(*foreground_, state, unlimited);
    if (foreground_->finished) {
      foreground_.reset();
    }
  }

  int budget = kMaxParallelCommandsPerFrame;
  for (Interpreter& interp : parallels_) {
    if (budget <= 0) {
      warnings_.push_back("Parallel command budget exhausted (32/frame)");
      break;
    }
    step_interpreter(interp, state, budget);
  }

  parallels_.erase(std::remove_if(parallels_.begin(), parallels_.end(),
                                  [](const Interpreter& i) { return i.finished; }),
                   parallels_.end());

  // Allow finished parallel events to restart next frame if still Parallel page.
  std::unordered_set<std::string> live;
  for (const Interpreter& interp : parallels_) {
    live.insert(interp.event_id);
  }
  for (auto it = parallel_started_.begin(); it != parallel_started_.end();) {
    if (!live.contains(*it)) {
      it = parallel_started_.erase(it);
    } else {
      ++it;
    }
  }

  // Autorun may re-fire only if conditions still match after unlock — unlock when finished.
  // Keep lock while conditions would still select autorun to avoid busy-loop without wait.
  for (auto it = autorun_lock_.begin(); it != autorun_lock_.end();) {
    const EventDef* event = nullptr;
    for (const EventDef& candidate : map_.events) {
      if (candidate.id == *it) {
        event = &candidate;
        break;
      }
    }
    if (event == nullptr) {
      it = autorun_lock_.erase(it);
      continue;
    }
    const int page_index = select_page(*event, state);
    const bool still_autorun =
        page_index >= 0 &&
        event->pages[static_cast<std::size_t>(page_index)].trigger == TriggerKind::Autorun;
    if (!still_autorun && !(foreground_.has_value() && foreground_->event_id == *it)) {
      it = autorun_lock_.erase(it);
    } else {
      ++it;
    }
  }

  active_parallel_count_ = static_cast<int>(parallels_.size());
}

}  // namespace rat
