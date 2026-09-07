#include "rat/event_runtime.hpp"

#include "rat/audio.hpp"
#include "rat/blocker_edit.hpp"
#include "rat/collision.hpp"
#include "rat/event_edit.hpp"
#include "rat/event_graph.hpp"
#include "rat/gameplay_notify.hpp"
#include "rat/map_document.hpp"

#include <algorithm>
#include <cmath>

namespace rat {
namespace {

constexpr float kEventHeightToleranceTiles = 0.35f;
constexpr float kPlayerBelowGroundEpsilon = 1e-4f;

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

Aabb2 tile_aabb(TileCoord tile, float tile_size) {
  const Vec3 center = tile_center_world(tile, tile_size);
  const float h = 0.5f * tile_size;
  return Aabb2{center.x - h, center.z - h, center.x + h, center.z + h};
}

Aabb2 centered_tile_aabb(float x, float z, float tile_size) {
  const float h = 0.5f * tile_size;
  return Aabb2{x - h, z - h, x + h, z + h};
}

TileCoord step_tile(TileCoord tile, RampDirection dir) {
  switch (dir) {
    case RampDirection::North:
      --tile.z;
      break;
    case RampDirection::East:
      ++tile.x;
      break;
    case RampDirection::South:
      ++tile.z;
      break;
    case RampDirection::West:
      --tile.x;
      break;
  }
  return tile;
}

constexpr const char* kGraphEntry = "entry";
constexpr const char* kGraphExit = "exit";

[[nodiscard]] std::string successor_or_exit(const EventGraph& graph, std::string_view from) {
  const std::optional<std::string> next = graph_sequence_successor(graph, from);
  return next.value_or(std::string(kGraphExit));
}

}  // namespace

EventRuntimeSnapshot EventRuntime::replay_snapshot() const {
  EventRuntimeSnapshot out;
  out.foreground = foreground_;
  out.parallels = parallels_;
  out.active_message = active_message_;
  auto sorted = [](const auto& set) {
    std::vector<std::string> values(set.begin(), set.end());
    std::sort(values.begin(), values.end());
    return values;
  };
  out.touch_inside = sorted(touch_inside_);
  out.parallel_started = sorted(parallel_started_);
  out.autorun_lock = sorted(autorun_lock_);
  out.overlays = {overlays_.begin(), overlays_.end()};
  out.have_last_player = have_last_player_;
  out.last_player_x = last_player_x_;
  out.last_player_y = last_player_y_;
  out.last_player_z = last_player_z_;
  return out;
}

void EventRuntime::load(const RuntimeMap& runtime) {
  const auto issues = validate_map_document(runtime.data);
  if (map_issues_have_errors(issues)) {
    warnings_.push_back("RuntimeMap rejected: " + format_map_issues(issues));
    return;
  }
  clear();
  runtime_map_ = runtime;
  surface_query_ = std::make_unique<SurfaceQuery>(runtime_map_.data);
  collision_world_ = bake_collision_world(runtime_map_.data, *surface_query_);
}

MapCompileResult EventRuntime::load(const MapData& map) {
  MapCompileResult compiled = compile_map_data(map);
  if (!compiled.ok) {
    return compiled;
  }
  load(compiled.runtime);
  return compiled;
}

void EventRuntime::set_audio(Audio* audio) {
  audio_ = audio;
}

void EventRuntime::set_notify(GameplayNotifyBus* notify) {
  notify_ = notify;
}

void EventRuntime::clear() {
  runtime_map_ = {};
  foreground_.reset();
  parallels_.clear();
  active_message_.reset();
  touch_inside_.clear();
  parallel_started_.clear();
  autorun_lock_.clear();
  active_parallel_count_ = 0;
  last_parallel_commands_executed_ = 0;
  last_commands_executed_ = 0;
  warnings_.clear();
  surface_query_.reset();
  collision_world_ = {};
  overlays_.clear();
  have_last_player_ = false;
  last_player_x_ = last_player_y_ = last_player_z_ = 0.0f;
  dt_ = 0.0f;
}

bool EventRuntime::player_input_blocked() const {
  return foreground_.has_value() || active_message_.has_value();
}

bool EventRuntime::has_action_prompt(const PlayerBody& player, const GameState& state) const {
  if (player_input_blocked()) {
    return false;
  }
  for (const EventDef& event : runtime_map_.data.events) {
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
  const auto overlay_it = overlays_.find(event.id);
  const bool has_overlay = overlay_it != overlays_.end();
  const float tile_size =
      runtime_map_.data.tile_size > 0.0f ? runtime_map_.data.tile_size : 1.0f;

  if (event.tile.has_value() && event.volume.has_value()) {
    Aabb2 volume = *event.volume;
    if (has_overlay) {
      const int dx = overlay_it->second.tile.x - event.tile->x;
      const int dz = overlay_it->second.tile.z - event.tile->z;
      volume = translate_aabb_on_grid(volume, dx, dz, tile_size);
    }
    return volume;
  }
  if (has_overlay) {
    return centered_tile_aabb(overlay_it->second.x, overlay_it->second.z, tile_size);
  }
  if (event.volume.has_value()) {
    return *event.volume;
  }
  if (event.tile.has_value()) {
    return tile_aabb(*event.tile, tile_size);
  }
  return Aabb2{0, 0, 0, 0};
}

SurfaceSample EventRuntime::event_surface_sample(const EventDef& event) const {
  SurfaceSample sample;
  if (!surface_query_) {
    return sample;
  }
  const Vec3 center = live_event_xz(event);
  return surface_query_->sample(center.x, center.z);
}

bool EventRuntime::event_height_matches_player(const EventDef& event, const PlayerBody& player) const {
  if (!surface_query_) {
    return true;
  }
  const float tile = runtime_map_.data.tile_size > 0.0f ? runtime_map_.data.tile_size : 1.0f;
  const float tolerance = kEventHeightToleranceTiles * tile;
  const SurfaceSample grid_event = event_surface_sample(event);
  const SurfaceSample grid_player = surface_query_->sample(player.x, player.z);
  if (grid_player.surface_id != grid_event.surface_id) {
    return false;
  }

  const float radius = player.half_extent > 0.0f ? player.half_extent : 0.4f;
  const std::optional<SolidSupport> player_support =
      query_solid_support(collision_world_, player.x, player.z, radius, player.y, 1.0e6f);

  float player_standing_y = grid_player.y;
  bool player_on_ramp = grid_player.on_ramp;
  int player_ramp_index = grid_player.ramp_index;
  if (player_support.has_value()) {
    player_standing_y = player_support->y;
    player_on_ramp = player_support->on_ramp;
    player_ramp_index = player_support->ramp_index;
  }

  Vec3 event_xz = live_event_xz(event);
  // Probe at authored y when set so a loft bind hits the slab; otherwise
  // height-grid Y skips loft boxes (feet_y < y_lo).
  const float event_probe_y = event.y.value_or(grid_event.y);
  const std::optional<SolidSupport> event_support =
      query_solid_support(collision_world_, event_xz.x, event_xz.z, 0.0f, event_probe_y, 1.0e6f);

  float event_y = grid_event.y;
  bool event_on_ramp = grid_event.on_ramp;
  int event_ramp_index = grid_event.ramp_index;
  if (event_support.has_value()) {
    event_y = event_support->y;
    event_on_ramp = event_support->on_ramp;
    event_ramp_index = event_support->ramp_index;
  }

  if (player.y + kPlayerBelowGroundEpsilon < player_standing_y) {
    return false;
  }
  if (player_on_ramp && event_on_ramp && player_ramp_index == event_ramp_index &&
      player_ramp_index >= 0) {
    return true;
  }
  return std::abs(player_standing_y - event_y) <= tolerance;
}

bool EventRuntime::player_overlaps(const EventDef& event, const PlayerBody& player) const {
  if (!event.volume.has_value() && !event.tile.has_value()) {
    return false;
  }
  if (!event_height_matches_player(event, player)) {
    return false;
  }
  const Aabb2 box = event_bounds(event);
  return circle_overlaps_aabb2(player.x, player.z, player.half_extent, box);
}

bool EventRuntime::action_in_range(const EventDef& event, const PlayerBody& player) const {
  if (!event_height_matches_player(event, player)) {
    return false;
  }
  if (event.tile.has_value() || overlays_.contains(event.id)) {
    const Vec3 center = live_event_xz(event);
    const float dx = player.x - center.x;
    const float dz = player.z - center.z;
    constexpr float kActionRadiusInTiles = 0.65f;
    const float radius = kActionRadiusInTiles * runtime_map_.data.tile_size;
    return dx * dx + dz * dz <= radius * radius;
  }
  // Explicit volumes keep authored AABB semantics.
  return event.volume.has_value() && player_overlaps(event, player);
}

void EventRuntime::start_page(const EventDef& event, int page_index, bool parallel, bool autorun) {
  EventPage* page = mutable_page(event.id, page_index);
  if (page != nullptr) {
    ensure_page_graph_from_commands(*page);
  }

  Interpreter interp;
  interp.event_id = event.id;
  interp.page_index = page_index;
  interp.parallel = parallel;
  interp.autorun = autorun;
  interp.node_id = kGraphExit;
  if (page != nullptr && page->graph.has_value()) {
    interp.node_id = successor_or_exit(*page->graph, kGraphEntry);
  }
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
  for (const EventDef& event : runtime_map_.data.events) {
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
  for (const EventDef& event : runtime_map_.data.events) {
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
  for (const EventDef& event : runtime_map_.data.events) {
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
  PlayerBody last_player = player;
  if (have_last_player_) {
    last_player.x = last_player_x_;
    last_player.y = last_player_y_;
    last_player.z = last_player_z_;
  }
  for (const EventDef& event : runtime_map_.data.events) {
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
    if (have_last_player_ && player_overlaps(event, last_player)) {
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
      if (notify_ != nullptr) {
        notify_->post({GameplayNotifyKind::DialogShown, {}});
      }
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
    case CommandOp::ConditionalBranch:
      return true;
    case CommandOp::Wait:
      interp.wait_frames = command.frames;
      return true;
    case CommandOp::TransferPlayer:
      if (command.map_id != runtime_map_.data.id || !std::isfinite(command.x) ||
          !std::isfinite(command.y) || !std::isfinite(command.z)) {
        warnings_.push_back("Unsupported or invalid Transfer Player ignored");
        interp.finished = true;
        return false;
      }
      state.set_map_id(command.map_id);
      if (surface_query_) {
        const float sampled_y = surface_query_->sample(command.x, command.z).y;
        state.set_player_position(command.x, sampled_y, command.z);
      } else {
        state.set_player_position(command.x, command.y, command.z);
      }
      return true;
    case CommandOp::ChangeItems:
      state.add_item(command.item_id, command.item_delta, command.key_item);
      if (notify_ != nullptr && command.item_delta > 0) {
        notify_->post({GameplayNotifyKind::ItemPicked, command.item_id});
      }
      return true;
    case CommandOp::PlaySE:
      if (audio_ != nullptr) {
        audio_->play_sfx(command.text);
      }
      return true;
    case CommandOp::SetMoveRoute:
      return true;
    case CommandOp::Comment:
      return true;
  }
  return true;
}

void EventRuntime::step_interpreter(Interpreter& interp, GameState& state,
                                    const PlayerBody& player, int& node_budget) {
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

  const EventGraph* graph = interpreter_graph(interp);
  if (graph == nullptr) {
    interp.finished = true;
    return;
  }

  while (true) {
    if (interp.parallel && node_budget <= 0) {
      return;
    }

    if (interp.node_id.empty() || interp.node_id == kGraphExit) {
      interp.finished = true;
      return;
    }
    if (interp.node_id == kGraphEntry) {
      interp.node_id = successor_or_exit(*graph, kGraphEntry);
      continue;
    }

    const EventGraphNode* node = find_graph_node(*graph, interp.node_id);
    if (node == nullptr) {
      interp.finished = true;
      return;
    }

    const auto pay_node = [&]() {
      ++last_commands_executed_;
      if (interp.parallel) {
        --node_budget;
        ++last_parallel_commands_executed_;
      }
    };

    if (node->kind == "set_move_route") {
      if (!interp.route_budget_paid) {
        pay_node();
        interp.route_budget_paid = true;
      }
      const Command command = command_from_node(*node);
      const bool done = exec_set_move_route(interp, command, player);
      if (done) {
        interp.node_id = successor_or_exit(*graph, node->id);
        interp.route_index = 0;
        interp.route_budget_paid = false;
        continue;
      }
      return;
    }

    if (node->kind == "conditional_branch") {
      pay_node();
      const bool ok = condition_met(node->branch_condition, state, interp.event_id);
      if (ok) {
        interp.node_id = graph_then_target(*graph, node->id).value_or(std::string(kGraphExit));
      } else if (const std::optional<std::string> else_to = graph_else_target(*graph, node->id)) {
        interp.node_id = *else_to;
      } else {
        interp.node_id = successor_or_exit(*graph, node->id);
      }
      continue;
    }

    pay_node();
    if (interp.parallel && node->kind == "comment" && node->text == "__nested_parallel__") {
      warnings_.push_back("Nested Parallel forbidden; ignored");
      interp.node_id = successor_or_exit(*graph, node->id);
      continue;
    }

    const Command command = command_from_node(*node);
    interp.node_id = successor_or_exit(*graph, node->id);

    const bool continue_now = exec_command(interp, state, command);
    if (!continue_now) {
      return;
    }
    if (interp.wait_frames > 0) {
      return;
    }
  }
}

void EventRuntime::update(GameState& state, const PlayerBody& player, bool interact_pressed,
                          float dt) {
  dt_ = dt > 0.0f ? dt : 0.0f;
  last_parallel_commands_executed_ = 0;
  last_commands_executed_ = 0;

  try_start_autorun(state);
  try_start_parallels(state);

  if (!player_input_blocked()) {
    try_start_action(state, player, interact_pressed);
    try_start_player_touch(state, player);
  }

  if (foreground_.has_value()) {
    int unlimited = 100000;
    step_interpreter(*foreground_, state, player, unlimited);
    if (foreground_->finished) {
      foreground_.reset();
    }
  }

  int budget = kMaxParallelCommandsPerFrame;
  for (Interpreter& interp : parallels_) {
    if (budget <= 0) {
      warnings_.push_back("Parallel node budget exhausted (32/frame)");
      break;
    }
    step_interpreter(interp, state, player, budget);
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
    for (const EventDef& candidate : runtime_map_.data.events) {
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
  last_player_x_ = player.x;
  last_player_y_ = player.y;
  last_player_z_ = player.z;
  have_last_player_ = true;
}

const EventDef* EventRuntime::find_event(std::string_view event_id) const {
  for (const EventDef& event : runtime_map_.data.events) {
    if (event.id == event_id) {
      return &event;
    }
  }
  return nullptr;
}

EventPage* EventRuntime::mutable_page(std::string_view event_id, int page_index) {
  if (page_index < 0) {
    return nullptr;
  }
  for (EventDef& event : runtime_map_.data.events) {
    if (event.id != event_id) {
      continue;
    }
    if (static_cast<std::size_t>(page_index) >= event.pages.size()) {
      return nullptr;
    }
    return &event.pages[static_cast<std::size_t>(page_index)];
  }
  return nullptr;
}

const EventPage* EventRuntime::interpreter_page(const Interpreter& interp) const {
  const EventDef* event = find_event(interp.event_id);
  if (event == nullptr || interp.page_index < 0) {
    return nullptr;
  }
  if (static_cast<std::size_t>(interp.page_index) >= event->pages.size()) {
    return nullptr;
  }
  return &event->pages[static_cast<std::size_t>(interp.page_index)];
}

const EventGraph* EventRuntime::interpreter_graph(const Interpreter& interp) const {
  const EventPage* page = interpreter_page(interp);
  if (page == nullptr || !page->graph.has_value()) {
    return nullptr;
  }
  return &*page->graph;
}

Vec3 EventRuntime::live_event_xz(const EventDef& event) const {
  const auto overlay_it = overlays_.find(event.id);
  if (overlay_it != overlays_.end()) {
    return Vec3{overlay_it->second.x, 0.0f, overlay_it->second.z};
  }
  if (event.tile.has_value()) {
    return tile_center_world(*event.tile, runtime_map_.data.tile_size);
  }
  if (event.volume.has_value()) {
    return Vec3{(event.volume->min_x + event.volume->max_x) * 0.5f, 0.0f,
                 (event.volume->min_z + event.volume->max_z) * 0.5f};
  }
  return {};
}

EventOverlay& EventRuntime::ensure_overlay(const EventDef& event) {
  const auto it = overlays_.find(event.id);
  if (it != overlays_.end()) {
    return it->second;
  }
  EventOverlay pose;
  if (event.tile.has_value()) {
    pose.tile = *event.tile;
    const float tile_size =
        runtime_map_.data.tile_size > 0.0f ? runtime_map_.data.tile_size : 1.0f;
    const Vec3 center = tile_center_world(pose.tile, tile_size);
    pose.x = center.x;
    pose.z = center.z;
  }
  return overlays_[event.id] = pose;
}

bool EventRuntime::tile_on_map(TileCoord tile) const {
  const MapData& map = runtime_map_.data;
  const HeightGrid& grid = map.height_grid;
  if (grid.width > 0 && grid.height > 0) {
    const int local_x = tile.x - grid.origin_x;
    const int local_z = tile.z - grid.origin_z;
    return local_x >= 0 && local_z >= 0 && local_x < grid.width && local_z < grid.height;
  }
  if (map.width > 0 && map.height > 0) {
    return tile.x >= 0 && tile.z >= 0 && tile.x < map.width && tile.z < map.height;
  }
  return true;
}

bool EventRuntime::dest_blocked(TileCoord dest, const PlayerBody& player, bool through,
                                 bool parallel) const {
  if (!tile_on_map(dest)) {
    return true;
  }
  if (through) {
    return false;
  }

  const float tile_size =
      runtime_map_.data.tile_size > 0.0f ? runtime_map_.data.tile_size : 1.0f;
  const Aabb2 dest_box = tile_aabb(dest, tile_size);
  const Vec3 center = tile_center_world(dest, tile_size);
  float feet_y = 0.0f;
  if (surface_query_) {
    feet_y = surface_query_->sample(center.x, center.z).y;
  }
  const std::optional<SolidSupport> support =
      query_solid_support(collision_world_, center.x, center.z, 0.0f, feet_y, 1.0e6f);
  if (support.has_value()) {
    feet_y = support->y;
  }

  if (parallel && circle_overlaps_aabb2(player.x, player.z, player.half_extent, dest_box)) {
    return true;
  }

  for (const BlockerDef& blocker : runtime_map_.data.blockers) {
    if (!blocker_blocks_feet(blocker, feet_y)) {
      continue;
    }
    if (aabb_overlap(dest_box, blocker.bounds)) {
      return true;
    }
  }

  CollisionBody probe;
  probe.x = center.x;
  probe.y = feet_y;
  probe.z = center.z;
  probe.radius = 0.5f * tile_size;
  probe.height = kPlayerCylinderHeight;
  if (cylinder_hits_fences(probe, collision_world_) ||
      cylinder_hits_walls(probe, collision_world_)) {
    return true;
  }
  return false;
}

bool EventRuntime::exec_set_move_route(Interpreter& interp, const Command& command,
                                         const PlayerBody& player) {
  const EventDef* event = find_event(interp.event_id);
  if (event == nullptr) {
    return true;
  }
  if (!event->tile.has_value()) {
    warnings_.push_back("set_move_route skipped volume-only event " + event->id);
    return true;
  }
  if (interp.route_index >= static_cast<int>(command.route.size())) {
    return true;
  }

  const RouteStep& step = command.route[static_cast<std::size_t>(interp.route_index)];
  switch (step.op) {
    case RouteStepOp::Wait:
      interp.wait_frames = step.frames;
      ++interp.route_index;
      return false;
    case RouteStepOp::Turn: {
      EventOverlay& pose = ensure_overlay(*event);
      pose.facing = step.dir;
      ++interp.route_index;
      return false;
    }
    case RouteStepOp::Move: {
      EventOverlay& pose = ensure_overlay(*event);
      const TileCoord dest = step_tile(pose.tile, step.dir);
      const float tile_size =
          runtime_map_.data.tile_size > 0.0f ? runtime_map_.data.tile_size : 1.0f;
      const Vec3 from = tile_center_world(pose.tile, tile_size);
      const bool at_start =
          std::abs(pose.x - from.x) < 1e-4f && std::abs(pose.z - from.z) < 1e-4f;
      if (at_start && dest_blocked(dest, player, command.through, interp.parallel)) {
        return false;
      }
      pose.facing = step.dir;
      const Vec3 dest_center = tile_center_world(dest, tile_size);
      const float dx = dest_center.x - pose.x;
      const float dz = dest_center.z - pose.z;
      const float dist = std::sqrt(dx * dx + dz * dz);
      const float speed = player.speed > 0.0f ? player.speed : 5.0f;
      const float step_len = speed * dt_;
      if (dist <= step_len + 1.0e-5f) {
        pose.x = dest_center.x;
        pose.z = dest_center.z;
        pose.tile = dest;
        ++interp.route_index;
        return false;
      }
      pose.x += dx * (step_len / dist);
      pose.z += dz * (step_len / dist);
      return false;
    }
  }
  ++interp.route_index;
  return false;
}

InterpreterDebug EventRuntime::to_debug(const Interpreter& interp) const {
  InterpreterDebug debug;
  debug.event_id = interp.event_id;
  debug.page_index = interp.page_index;
  debug.command_index = 0;
  if (const EventGraph* graph = interpreter_graph(interp)) {
    for (std::size_t i = 0; i < graph->nodes.size(); ++i) {
      if (graph->nodes[i].id == interp.node_id) {
        debug.command_index = static_cast<int>(i);
        break;
      }
    }
  }
  debug.wait_frames = interp.wait_frames;
  debug.route_index = interp.route_index;
  debug.waiting_message = interp.waiting_message;
  debug.parallel = interp.parallel;
  return debug;
}

std::optional<EventOverlay> EventRuntime::event_overlay(std::string_view event_id) const {
  const auto it = overlays_.find(std::string(event_id));
  if (it == overlays_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<Vec3> EventRuntime::event_markers() const {
  std::vector<Vec3> markers;
  markers.reserve(runtime_map_.data.events.size());
  for (const EventDef& event : runtime_map_.data.events) {
    if (!event.tile.has_value() && !event.volume.has_value()) {
      continue;
    }
    Vec3 marker = live_event_xz(event);
    const bool overlay_live = overlays_.contains(event.id);
    // Spawn/unmoved: sit on EventDef bind Y. Overlay-live keeps dest surface sample.
    if (event.y.has_value() && !overlay_live) {
      marker.y = *event.y;
    } else if (surface_query_) {
      marker.y = surface_query_->sample(marker.x, marker.z).y;
    }
    markers.push_back(marker);
  }
  return markers;
}

std::vector<std::string> EventRuntime::overlapping_event_ids(const PlayerBody& player) const {
  std::vector<std::string> ids;
  for (const EventDef& event : runtime_map_.data.events) {
    if (player_overlaps(event, player)) {
      ids.push_back(event.id);
    }
  }
  return ids;
}

std::optional<InterpreterDebug> EventRuntime::foreground_debug() const {
  if (!foreground_.has_value()) {
    return std::nullopt;
  }
  return to_debug(*foreground_);
}

std::vector<InterpreterDebug> EventRuntime::parallel_debug() const {
  std::vector<InterpreterDebug> out;
  out.reserve(parallels_.size());
  for (const Interpreter& interp : parallels_) {
    out.push_back(to_debug(interp));
  }
  return out;
}

const char* event_why_not_name(EventWhyNot reason) {
  switch (reason) {
    case EventWhyNot::Ok:
      return "ok";
    case EventWhyNot::WrongPage:
      return "wrong_page";
    case EventWhyNot::Conditions:
      return "conditions";
    case EventWhyNot::Height:
      return "height";
    case EventWhyNot::NotOverlapping:
      return "not_overlapping";
    case EventWhyNot::OutOfActionRange:
      return "out_of_action_range";
    case EventWhyNot::InputBlocked:
      return "input_blocked";
    case EventWhyNot::AlreadyRunning:
      return "already_running";
    case EventWhyNot::AutorunLock:
      return "autorun_lock";
    case EventWhyNot::ForegroundBusy:
      return "foreground_busy";
    case EventWhyNot::ParallelLimit:
      return "parallel_limit";
    case EventWhyNot::AlreadyInside:
      return "already_inside";
  }
  return "not_overlapping";
}

EventWhyNot EventRuntime::why_not_fired(std::string_view event_id, const GameState& state,
                                         const PlayerBody& player, bool interact_pressed) const {
  const EventDef* event = nullptr;
  for (const EventDef& candidate : runtime_map_.data.events) {
    if (candidate.id == event_id) {
      event = &candidate;
      break;
    }
  }
  if (event == nullptr) {
    return EventWhyNot::NotOverlapping;
  }

  if (foreground_.has_value() && foreground_->event_id == event_id) {
    return EventWhyNot::AlreadyRunning;
  }
  for (const Interpreter& interp : parallels_) {
    if (interp.event_id == event_id) {
      return EventWhyNot::AlreadyRunning;
    }
  }

  const int page_index = select_page(*event, state);
  if (page_index < 0) {
    return EventWhyNot::Conditions;
  }

  const TriggerKind trigger = event->pages[static_cast<std::size_t>(page_index)].trigger;
  const bool action_or_touch =
      trigger == TriggerKind::Action || trigger == TriggerKind::PlayerTouch;
  if (trigger == TriggerKind::EventTouch) {
    return EventWhyNot::WrongPage;
  }

  if (action_or_touch && player_input_blocked()) {
    return EventWhyNot::InputBlocked;
  }

  const bool has_place = event->tile.has_value() || event->volume.has_value();
  if (action_or_touch && has_place && !event_height_matches_player(*event, player)) {
    return EventWhyNot::Height;
  }

  if (action_or_touch && !player_overlaps(*event, player)) {
    return EventWhyNot::NotOverlapping;
  }

  if (trigger == TriggerKind::Action) {
    if (!action_in_range(*event, player) || !interact_pressed) {
      return EventWhyNot::OutOfActionRange;
    }
  }

  if (trigger == TriggerKind::Autorun) {
    if (player_input_blocked()) {
      return EventWhyNot::ForegroundBusy;
    }
    if (autorun_lock_.contains(event->id)) {
      return EventWhyNot::AutorunLock;
    }
  }

  if (trigger == TriggerKind::Parallel) {
    if (static_cast<int>(parallels_.size()) >= kMaxParallelEvents) {
      return EventWhyNot::ParallelLimit;
    }
  }

  if (trigger == TriggerKind::PlayerTouch && touch_inside_.contains(event->id)) {
    return EventWhyNot::AlreadyInside;
  }

  return EventWhyNot::Ok;
}

EventWhyNot event_why_not_fired(const EventRuntime& runtime, std::string_view event_id,
                                 const GameState& state, const PlayerBody& player,
                                 bool interact_pressed) {
  return runtime.why_not_fired(event_id, state, player, interact_pressed);
}

}  // namespace rat
