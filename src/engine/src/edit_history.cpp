#include "rat/edit_history.hpp"

#include "rat/blocker_edit.hpp"
#include "rat/event_edit.hpp"

#include <cstddef>
#include <utility>

namespace rat {

namespace {

EditApplyResult result_from(const EditCommand& command) {
  EditApplyResult result;
  result.applied = true;
  result.mutates_blockers = command.mutates_blockers();
  result.mutates_events = command.mutates_events();
  return result;
}

class PlaceBlockerCommand final : public EditCommand {
 public:
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

class DeleteBlockerCommand final : public EditCommand {
 public:
  explicit DeleteBlockerCommand(std::size_t index) : index_(index) {}

  void apply(MapData& map) override {
    if (index_ >= map.blockers.size()) {
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

class MoveBlockerCommand final : public EditCommand {
 public:
  MoveBlockerCommand(std::size_t index, int tile_dx, int tile_dz, float tile_size)
      : index_(index), tile_dx_(tile_dx), tile_dz_(tile_dz), tile_size_(tile_size) {}

  void apply(MapData& map) override { translate(map, tile_dx_, tile_dz_); }

  void revert(MapData& map) override { translate(map, -tile_dx_, -tile_dz_); }

  [[nodiscard]] bool mutates_blockers() const override { return true; }
  [[nodiscard]] bool mutates_events() const override { return false; }

 private:
  void translate(MapData& map, int dx, int dz) const {
    if (index_ >= map.blockers.size()) {
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

class ReplaceBlockerCommand final : public EditCommand {
 public:
  ReplaceBlockerCommand(std::size_t index, BlockerDef next)
      : index_(index), next_(std::move(next)) {}

  void apply(MapData& map) override {
    if (index_ >= map.blockers.size()) {
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

class PlaceEventCommand final : public EditCommand {
 public:
  explicit PlaceEventCommand(EventDef event) : event_(std::move(event)) {}

  void apply(MapData& map) override {
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

class DeleteEventCommand final : public EditCommand {
 public:
  explicit DeleteEventCommand(std::size_t index) : index_(index) {}

  void apply(MapData& map) override {
    if (index_ >= map.events.size()) {
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

class MoveEventCommand final : public EditCommand {
 public:
  MoveEventCommand(std::size_t index, int tile_dx, int tile_dz, float tile_size)
      : index_(index), tile_dx_(tile_dx), tile_dz_(tile_dz), tile_size_(tile_size) {}

  void apply(MapData& map) override { translate(map, tile_dx_, tile_dz_); }

  void revert(MapData& map) override { translate(map, -tile_dx_, -tile_dz_); }

  [[nodiscard]] bool mutates_blockers() const override { return false; }
  [[nodiscard]] bool mutates_events() const override { return true; }

 private:
  void translate(MapData& map, int dx, int dz) const {
    if (index_ >= map.events.size()) {
      return;
    }
    translate_event_on_grid(map.events[index_], dx, dz, tile_size_);
  }

  std::size_t index_ = 0;
  int tile_dx_ = 0;
  int tile_dz_ = 0;
  float tile_size_ = 1.0f;
};

class ReplaceEventCommand final : public EditCommand {
 public:
  ReplaceEventCommand(std::size_t index, EventDef next) : index_(index), next_(std::move(next)) {}

  void apply(MapData& map) override {
    if (index_ >= map.events.size()) {
      captured_ = false;
      return;
    }
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

EditApplyResult EditHistory::execute(MapData& map, std::unique_ptr<EditCommand> command) {
  if (command == nullptr) {
    return {};
  }
  command->apply(map);
  const EditApplyResult result = result_from(*command);
  undo_.push_back(std::move(command));
  redo_.clear();
  return result;
}

EditApplyResult EditHistory::undo(MapData& map) {
  if (undo_.empty()) {
    return {};
  }
  std::unique_ptr<EditCommand> command = std::move(undo_.back());
  undo_.pop_back();
  command->revert(map);
  const EditApplyResult result = result_from(*command);
  redo_.push_back(std::move(command));
  return result;
}

EditApplyResult EditHistory::redo(MapData& map) {
  if (redo_.empty()) {
    return {};
  }
  std::unique_ptr<EditCommand> command = std::move(redo_.back());
  redo_.pop_back();
  command->apply(map);
  const EditApplyResult result = result_from(*command);
  undo_.push_back(std::move(command));
  return result;
}

void EditHistory::clear() {
  undo_.clear();
  redo_.clear();
}

bool EditHistory::can_undo() const {
  return !undo_.empty();
}

bool EditHistory::can_redo() const {
  return !redo_.empty();
}

}  // namespace rat
