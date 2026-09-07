#include "rat/edit_history.hpp"
#include "rat/retained_memory.hpp"

#include "rat/blocker_edit.hpp"
#include "rat/authoring_snapshot.hpp"
#include "rat/event_edit.hpp"
#include "rat/height_edit.hpp"

#include <cstddef>
#include <functional>
#include <utility>

namespace rat {

namespace {

EditApplyResult result_from(const EditCommand& command) {
  EditApplyResult result;
  result.ok = true;
  result.changed = true;
  result.mutates_blockers = command.mutates_blockers();
  result.mutates_events = command.mutates_events();
  result.mutates_elevation = command.mutates_elevation();
  return result;
}

class CheckedCommand : public EditCommand {
 public:
  [[nodiscard]] bool applied_successfully() const override { return error_.empty(); }
  [[nodiscard]] std::string last_error() const override { return error_; }
 protected:
  std::string error_;
  bool index_valid(std::size_t index, std::size_t size) {
    error_ = index < size ? "" : "edit index is out of range";
    return error_.empty();
  }
  bool event_id_valid(const MapData& map, const std::string& id, std::size_t excluded = static_cast<std::size_t>(-1)) {
    error_.clear();
    if (id.empty()) error_ = "event id must not be empty";
    for (std::size_t i = 0; i < map.events.size(); ++i)
      if (i != excluded && map.events[i].id == id) error_ = "event id already exists: " + id;
    return error_.empty();
  }
};

class PlaceBlockerCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_);
  }
  explicit PlaceBlockerCommand(BlockerDef blocker) : blocker_(std::move(blocker)) {}

  void apply(MapData& map) override {
    index_ = map.blockers.size();
    map.blockers.push_back(blocker_);
  }

  void revert(MapData& map) override {
    if (index_ < map.blockers.size()) {
      map.blockers.erase(map.blockers.begin() + static_cast<std::ptrdiff_t>(index_));
    }
  }

  [[nodiscard]] bool mutates_blockers() const override { return true; }
  [[nodiscard]] bool mutates_events() const override { return false; }

 private:
  BlockerDef blocker_{};
  std::size_t index_ = 0;
};

class DeleteBlockerCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_);
  }
  explicit DeleteBlockerCommand(std::size_t index) : index_(index) {}

  void apply(MapData& map) override {
    if (!index_valid(index_, map.blockers.size())) {
      captured_ = false;
      return;
    }
    removed_ = map.blockers[index_];
    captured_ = true;
    map.blockers.erase(map.blockers.begin() + static_cast<std::ptrdiff_t>(index_));
  }

  void revert(MapData& map) override {
    if (!captured_ || index_ > map.blockers.size()) {
      return;
    }
    map.blockers.insert(map.blockers.begin() + static_cast<std::ptrdiff_t>(index_), removed_);
  }

  [[nodiscard]] bool mutates_blockers() const override { return true; }
  [[nodiscard]] bool mutates_events() const override { return false; }

 private:
  std::size_t index_ = 0;
  BlockerDef removed_{};
  bool captured_ = false;
};

class MoveBlockerCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_);
  }
  MoveBlockerCommand(std::size_t index, int tile_dx, int tile_dz, float tile_size)
      : index_(index), tile_dx_(tile_dx), tile_dz_(tile_dz), tile_size_(tile_size) {}

  void apply(MapData& map) override { translate(map, tile_dx_, tile_dz_); }

  void revert(MapData& map) override { translate(map, -tile_dx_, -tile_dz_); }

  [[nodiscard]] bool mutates_blockers() const override { return true; }
  [[nodiscard]] bool mutates_events() const override { return false; }

 private:
  void translate(MapData& map, int dx, int dz) {
    if (!index_valid(index_, map.blockers.size())) {
      return;
    }
    BlockerDef& blocker = map.blockers[index_];
    blocker.bounds = translate_aabb_on_grid(blocker.bounds, dx, dz, tile_size_);
  }

  std::size_t index_ = 0;
  int tile_dx_ = 0;
  int tile_dz_ = 0;
  float tile_size_ = 1.0f;
};

class ReplaceBlockerCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_);
  }
  ReplaceBlockerCommand(std::size_t index, BlockerDef next)
      : index_(index), next_(std::move(next)) {}

  void apply(MapData& map) override {
    if (!index_valid(index_, map.blockers.size())) {
      captured_ = false;
      return;
    }
    previous_ = map.blockers[index_];
    captured_ = true;
    map.blockers[index_] = next_;
  }

  void revert(MapData& map) override {
    if (!captured_ || index_ >= map.blockers.size()) {
      return;
    }
    map.blockers[index_] = previous_;
  }

  [[nodiscard]] bool mutates_blockers() const override { return true; }
  [[nodiscard]] bool mutates_events() const override { return false; }

 private:
  std::size_t index_ = 0;
  BlockerDef next_{};
  BlockerDef previous_{};
  bool captured_ = false;
};

class PlaceEventCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_) + retained_dynamic_bytes(event_);
  }
  explicit PlaceEventCommand(EventDef event) : event_(std::move(event)) {}

  void apply(MapData& map) override {
    if (!event_id_valid(map, event_.id)) return;
    index_ = map.events.size();
    map.events.push_back(event_);
  }

  void revert(MapData& map) override {
    if (index_ < map.events.size()) {
      map.events.erase(map.events.begin() + static_cast<std::ptrdiff_t>(index_));
    }
  }

  [[nodiscard]] bool mutates_blockers() const override { return false; }
  [[nodiscard]] bool mutates_events() const override { return true; }

 private:
  EventDef event_{};
  std::size_t index_ = 0;
};

class DeleteEventCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_) + retained_dynamic_bytes(removed_);
  }
  explicit DeleteEventCommand(std::size_t index) : index_(index) {}

  void apply(MapData& map) override {
    if (!index_valid(index_, map.events.size())) {
      captured_ = false;
      return;
    }
    removed_ = map.events[index_];
    captured_ = true;
    map.events.erase(map.events.begin() + static_cast<std::ptrdiff_t>(index_));
  }

  void revert(MapData& map) override {
    if (!captured_ || index_ > map.events.size()) {
      return;
    }
    map.events.insert(map.events.begin() + static_cast<std::ptrdiff_t>(index_), removed_);
  }

  [[nodiscard]] bool mutates_blockers() const override { return false; }
  [[nodiscard]] bool mutates_events() const override { return true; }

 private:
  std::size_t index_ = 0;
  EventDef removed_{};
  bool captured_ = false;
};

class MoveEventCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_);
  }
  MoveEventCommand(std::size_t index, int tile_dx, int tile_dz, float tile_size)
      : index_(index), tile_dx_(tile_dx), tile_dz_(tile_dz), tile_size_(tile_size) {}

  void apply(MapData& map) override { translate(map, tile_dx_, tile_dz_); }

  void revert(MapData& map) override { translate(map, -tile_dx_, -tile_dz_); }

  [[nodiscard]] bool mutates_blockers() const override { return false; }
  [[nodiscard]] bool mutates_events() const override { return true; }

 private:
  void translate(MapData& map, int dx, int dz) {
    if (!index_valid(index_, map.events.size())) {
      return;
    }
    translate_event_on_grid(map.events[index_], dx, dz, tile_size_);
  }

  std::size_t index_ = 0;
  int tile_dx_ = 0;
  int tile_dz_ = 0;
  float tile_size_ = 1.0f;
};

class ReplaceEventCommand final : public CheckedCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(error_) + retained_dynamic_bytes(next_) + retained_dynamic_bytes(previous_);
  }
  ReplaceEventCommand(std::size_t index, EventDef next) : index_(index), next_(std::move(next)) {}

  void apply(MapData& map) override {
    if (!index_valid(index_, map.events.size())) {
      captured_ = false;
      return;
    }
    if (!event_id_valid(map, next_.id, index_)) { captured_ = false; return; }
    previous_ = map.events[index_];
    captured_ = true;
    map.events[index_] = next_;
  }

  void revert(MapData& map) override {
    if (!captured_ || index_ >= map.events.size()) {
      return;
    }
    map.events[index_] = previous_;
  }

  [[nodiscard]] bool mutates_blockers() const override { return false; }
  [[nodiscard]] bool mutates_events() const override { return true; }

 private:
  std::size_t index_ = 0;
  EventDef next_{};
  EventDef previous_{};
  bool captured_ = false;
};

class ReplaceElevationSnapshotCommand final : public EditCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    return sizeof(*this) + retained_dynamic_bytes(last_error_)
        + retained_vector_bytes(before_height_grid_.ground_y)
        + retained_vector_bytes(before_ramps_)
        + retained_vector_bytes(before_edge_barriers_)
        + retained_vector_bytes(before_floor_slabs_)
        + retained_vector_bytes(before_ladders_)
        + retained_vector_bytes(before_occupancy_)
        + retained_vector_bytes(after_height_grid_.ground_y)
        + retained_vector_bytes(after_ramps_)
        + retained_vector_bytes(after_edge_barriers_)
        + retained_vector_bytes(after_floor_slabs_)
        + retained_vector_bytes(after_ladders_)
        + retained_vector_bytes(after_occupancy_);
  }
  using ElevationEditFn = std::function<HeightEditResult(MapData&)>;

  explicit ReplaceElevationSnapshotCommand(ElevationEditFn edit) : edit_(std::move(edit)) {}

  void apply(MapData& map) override {
    if (committed_) {
      map.schema_version = after_schema_version_;
      map.height_grid = after_height_grid_;
      map.ramps = after_ramps_;
      map.edge_barriers = after_edge_barriers_;
      map.floor_slabs = after_floor_slabs_;
      map.ladders = after_ladders_;
      map.occupancy = after_occupancy_;
      applied_successfully_ = true;
      return;
    }
    before_schema_version_ = map.schema_version;
    before_height_grid_ = map.height_grid;
    before_ramps_ = map.ramps;
    before_edge_barriers_ = map.edge_barriers;
    before_floor_slabs_ = map.floor_slabs;
    before_ladders_ = map.ladders;
    before_occupancy_ = map.occupancy;

    const HeightEditResult edited = edit_(map);
    if (!edited.ok) {
      map.schema_version = before_schema_version_;
      map.height_grid = before_height_grid_;
      map.ramps = before_ramps_;
      map.edge_barriers = before_edge_barriers_;
      map.floor_slabs = before_floor_slabs_;
      map.ladders = before_ladders_;
      map.occupancy = before_occupancy_;
      applied_successfully_ = false;
      last_error_ = edited.error;
      return;
    }
    after_schema_version_ = map.schema_version;
    after_height_grid_ = map.height_grid;
    after_ramps_ = map.ramps;
    after_edge_barriers_ = map.edge_barriers;
    after_floor_slabs_ = map.floor_slabs;
    after_ladders_ = map.ladders;
    after_occupancy_ = map.occupancy;
    committed_ = true;
    applied_successfully_ = true;
    last_error_.clear();
  }

  void revert(MapData& map) override {
    if (!committed_) {
      return;
    }
    map.schema_version = before_schema_version_;
    map.height_grid = before_height_grid_;
    map.ramps = before_ramps_;
    map.edge_barriers = before_edge_barriers_;
    map.floor_slabs = before_floor_slabs_;
    map.ladders = before_ladders_;
    map.occupancy = before_occupancy_;
  }

  [[nodiscard]] bool applied_successfully() const override { return applied_successfully_; }
  [[nodiscard]] std::string last_error() const override { return last_error_; }
  [[nodiscard]] bool mutates_blockers() const override { return false; }
  [[nodiscard]] bool mutates_events() const override { return false; }
  [[nodiscard]] bool mutates_elevation() const override { return true; }

 private:
  ElevationEditFn edit_{};
  int before_schema_version_ = 1;
  HeightGrid before_height_grid_{};
  std::vector<RampDef> before_ramps_{};
  std::vector<EdgeBarrierDef> before_edge_barriers_{};
  std::vector<FloorSlabDef> before_floor_slabs_{};
  std::vector<LadderDef> before_ladders_{};
  std::vector<OccupancyCell> before_occupancy_{};
  int after_schema_version_ = 1;
  HeightGrid after_height_grid_{};
  std::vector<RampDef> after_ramps_{};
  std::vector<EdgeBarrierDef> after_edge_barriers_{};
  std::vector<FloorSlabDef> after_floor_slabs_{};
  std::vector<LadderDef> after_ladders_{};
  std::vector<OccupancyCell> after_occupancy_{};
  bool committed_ = false;
  bool applied_successfully_ = false;
  std::string last_error_{};
};

class CompositeCommand final : public EditCommand {
 public:
  std::size_t estimated_retained_bytes() const override {
    auto n = sizeof(*this) + retained_dynamic_bytes(error_) + retained_vector_bytes(children_);
    for (const auto& child : children_) n += child->estimated_retained_bytes();
    return n;
  }
  void append(std::unique_ptr<EditCommand> command) {
    if (command != nullptr) {
      children_.push_back(std::move(command));
    }
  }

  [[nodiscard]] bool empty() const { return children_.empty(); }

  void apply(MapData& map) override {
    for (std::unique_ptr<EditCommand>& child : children_) {
      child->apply(map);
      if (!child->applied_successfully()) { ok_ = false; error_ = child->last_error(); return; }
    }
    ok_ = true;
    error_.clear();
  }

  void revert(MapData& map) override {
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
      (*it)->revert(map);
    }
  }

  [[nodiscard]] bool applied_successfully() const override { return ok_ && !children_.empty(); }

  [[nodiscard]] std::string last_error() const override { return error_; }

  [[nodiscard]] bool mutates_blockers() const override {
    for (const std::unique_ptr<EditCommand>& child : children_) {
      if (child->mutates_blockers()) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool mutates_events() const override {
    for (const std::unique_ptr<EditCommand>& child : children_) {
      if (child->mutates_events()) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool mutates_elevation() const override {
    for (const std::unique_ptr<EditCommand>& child : children_) {
      if (child->mutates_elevation()) {
        return true;
      }
    }
    return false;
  }

 private:
  bool ok_ = true;
  std::string error_;
  std::vector<std::unique_ptr<EditCommand>> children_{};
};

std::unique_ptr<EditCommand> take_stroke_command(std::vector<std::unique_ptr<EditCommand>>& stroke) {
  if (stroke.empty()) {
    return nullptr;
  }
  if (stroke.size() == 1) {
    std::unique_ptr<EditCommand> command = std::move(stroke.front());
    stroke.clear();
    return command;
  }
  auto composite = std::make_unique<CompositeCommand>();
  for (std::unique_ptr<EditCommand>& child : stroke) {
    composite->append(std::move(child));
  }
  stroke.clear();
  return composite;
}

}  // namespace

std::unique_ptr<EditCommand> make_place_blocker_command(BlockerDef blocker) {
  return std::make_unique<PlaceBlockerCommand>(std::move(blocker));
}

std::unique_ptr<EditCommand> make_delete_blocker_command(std::size_t index) {
  return std::make_unique<DeleteBlockerCommand>(index);
}

std::unique_ptr<EditCommand> make_move_blocker_command(std::size_t index, int tile_dx, int tile_dz,
                                                       float tile_size) {
  return std::make_unique<MoveBlockerCommand>(index, tile_dx, tile_dz, tile_size);
}

std::unique_ptr<EditCommand> make_replace_blocker_command(std::size_t index, BlockerDef next) {
  return std::make_unique<ReplaceBlockerCommand>(index, std::move(next));
}

std::unique_ptr<EditCommand> make_place_event_command(EventDef event) {
  return std::make_unique<PlaceEventCommand>(std::move(event));
}

std::unique_ptr<EditCommand> make_duplicate_event_command(const MapData& map, std::size_t index) {
  if (index >= map.events.size()) return nullptr;
  return make_place_event_command(make_duplicate_event(map, index));
}

std::unique_ptr<EditCommand> make_delete_event_command(std::size_t index) {
  return std::make_unique<DeleteEventCommand>(index);
}

std::unique_ptr<EditCommand> make_move_event_command(std::size_t index, int tile_dx, int tile_dz,
                                                     float tile_size) {
  return std::make_unique<MoveEventCommand>(index, tile_dx, tile_dz, tile_size);
}

std::unique_ptr<EditCommand> make_replace_event_command(std::size_t index, EventDef next) {
  return std::make_unique<ReplaceEventCommand>(index, std::move(next));
}

std::unique_ptr<EditCommand> make_set_map_tile_ground_y_command(int tile_x, int tile_z,
                                                                 float ground_y) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [tile_x, tile_z, ground_y](MapData& map) {
        return set_map_tile_ground_y(map, tile_x, tile_z, ground_y);
      });
}

std::unique_ptr<EditCommand> make_adjust_map_tile_ground_y_command(int tile_x, int tile_z,
                                                                    float delta_y) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [tile_x, tile_z, delta_y](MapData& map) {
        return adjust_map_tile_ground_y(map, tile_x, tile_z, delta_y);
      });
}

std::unique_ptr<EditCommand> make_place_map_tile_cube_command(int tile_x, int tile_z) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [tile_x, tile_z](MapData& map) { return place_map_tile_cube(map, tile_x, tile_z); });
}

std::unique_ptr<EditCommand> make_upsert_map_ramp_command(RampDef ramp) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [ramp = std::move(ramp)](MapData& map) { return upsert_map_ramp(map, ramp); });
}

std::unique_ptr<EditCommand> make_remove_map_ramp_command(TileCoord tile) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [tile](MapData& map) { return remove_map_ramp(map, tile); });
}

std::unique_ptr<EditCommand> make_upsert_map_edge_barrier_command(EdgeBarrierDef edge) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [edge = std::move(edge)](MapData& map) { return upsert_map_edge_barrier(map, edge); });
}

std::unique_ptr<EditCommand> make_remove_map_edge_barrier_command(TileCoord tile,
                                                                   RampDirection direction) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [tile, direction](MapData& map) { return remove_map_edge_barrier(map, tile, direction); });
}

std::unique_ptr<EditCommand> make_upsert_map_floor_slab_command(FloorSlabDef slab) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [slab = std::move(slab)](MapData& map) { return upsert_map_floor_slab(map, slab); });
}

std::unique_ptr<EditCommand> make_upsert_map_ladder_command(LadderDef ladder) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [ladder = std::move(ladder)](MapData& map) { return upsert_map_ladder(map, ladder); });
}

std::unique_ptr<EditCommand> make_remove_map_ladder_command(TileCoord tile,
                                                             RampDirection direction) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [tile, direction](MapData& map) { return remove_map_ladder(map, tile, direction); });
}

std::unique_ptr<EditCommand> make_place_map_occupancy_solid_command(int x, int y, int z) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [x, y, z](MapData& map) { return place_map_occupancy_solid(map, x, y, z); });
}

std::unique_ptr<EditCommand> make_place_map_occupancy_ramp_command(int x, int y, int z,
                                                                   RampDirection yaw) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [x, y, z, yaw](MapData& map) { return place_map_occupancy_ramp(map, x, y, z, yaw); });
}

std::unique_ptr<EditCommand> make_remove_map_occupancy_cell_command(int x, int y, int z) {
  return std::make_unique<ReplaceElevationSnapshotCommand>(
      [x, y, z](MapData& map) { return remove_map_occupancy_cell(map, x, y, z); });
}

EditApplyResult EditHistory::execute(MapData& map, std::unique_ptr<EditCommand> command) {
  if (!command) return {false, false, false, false, false, "missing edit command"};
  MapData candidate = map;
  command->apply(candidate);
  if (!command->applied_successfully()) {
    return {false, false, false, false, false, command->last_error()};
  }
  if (authoring_snapshot(candidate) == authoring_snapshot(map)) return {true, false};
  const auto result = result_from(*command);
  if (in_stroke_) {
    if (stroke_.empty()) stroke_before_ = authoring_snapshot(map);
    stroke_after_ = authoring_snapshot(candidate);
  }
  map = std::move(candidate);
  if (in_stroke_) stroke_.push_back(std::move(command));
  else {
    undo_.push_back(std::move(command));
    redo_.clear();
  }
  return result;
}

EditApplyResult EditHistory::undo(MapData& map) {
  if (in_stroke_) {
    if (!stroke_.empty()) return abort_stroke(map);
    in_stroke_ = false;
  }
  if (undo_.empty()) return {};
  MapData candidate = map;
  undo_.back()->revert(candidate);
  const auto result = result_from(*undo_.back());
  map = std::move(candidate);
  redo_.push_back(std::move(undo_.back()));
  undo_.pop_back();
  return result;
}

EditApplyResult EditHistory::redo(MapData& map) {
  if (in_stroke_ || redo_.empty()) return {};
  MapData candidate = map;
  auto& command = redo_.back();
  command->apply(candidate);
  if (!command->applied_successfully()) {
    return {false, false, false, false, false, command->last_error()};
  }
  auto result = result_from(*command);
  result.changed = authoring_snapshot(candidate) != authoring_snapshot(map);
  if (!result.changed) result.mutates_blockers = result.mutates_events = result.mutates_elevation = false;
  map = std::move(candidate);
  undo_.push_back(std::move(command));
  redo_.pop_back();
  return result;
}

void EditHistory::begin_stroke() {
  if (in_stroke_) {
    return;
  }
  in_stroke_ = true;
  stroke_.clear();
  stroke_before_.clear();
  stroke_after_.clear();
}

void EditHistory::end_stroke() {
  if (!in_stroke_) {
    return;
  }
  in_stroke_ = false;
  if (stroke_before_ == stroke_after_) { stroke_.clear(); return; }
  std::unique_ptr<EditCommand> command = take_stroke_command(stroke_);
  if (command == nullptr) {
    return;
  }
  undo_.push_back(std::move(command));
  redo_.clear();
}

EditApplyResult EditHistory::abort_stroke(MapData& map) {
  if (!in_stroke_) {
    return {};
  }
  in_stroke_ = false;
  if (stroke_.empty()) {
    return {};
  }
  std::unique_ptr<EditCommand> command = take_stroke_command(stroke_);
  const auto before = authoring_snapshot(map);
  command->revert(map);
  if (before == authoring_snapshot(map)) return {true, false};
  return result_from(*command);
}

void EditHistory::clear() {
  undo_.clear();
  redo_.clear();
  stroke_.clear();
  in_stroke_ = false;
}

bool EditHistory::can_undo() const {
  return !undo_.empty() || (in_stroke_ && !stroke_.empty());
}

bool EditHistory::can_redo() const {
  return !redo_.empty();
}

bool EditHistory::in_stroke() const {
  return in_stroke_;
}

EditHistoryMemory EditHistory::estimated_retained_memory() const {
  EditHistoryMemory result;
  result.object_bytes = sizeof(*this);
  result.queue_capacity_bytes = retained_vector_bytes(undo_) + retained_vector_bytes(redo_)
      + retained_vector_bytes(stroke_);
  for (const auto& c : undo_) result.undo_commands_bytes += c->estimated_retained_bytes();
  for (const auto& c : redo_) result.redo_commands_bytes += c->estimated_retained_bytes();
  for (const auto& c : stroke_) result.stroke_commands_bytes += c->estimated_retained_bytes();
  result.stroke_buffers_bytes = retained_dynamic_bytes(stroke_before_) + retained_dynamic_bytes(stroke_after_);
  return result;
}

}  // namespace rat
