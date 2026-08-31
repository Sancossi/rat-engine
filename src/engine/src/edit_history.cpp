#include "rat/edit_history.hpp"

#include "rat/blocker_edit.hpp"
#include "rat/event_edit.hpp"

#include <cstddef>
#include <utility>

namespace rat {

namespace {

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

void EditHistory::execute(MapData& map, std::unique_ptr<EditCommand> command) {
  if (command == nullptr) {
    return;
  }
  command->apply(map);
  undo_.push_back(std::move(command));
  redo_.clear();
}

bool EditHistory::undo(MapData& map) {
  if (undo_.empty()) {
    return false;
  }
  std::unique_ptr<EditCommand> command = std::move(undo_.back());
  undo_.pop_back();
  command->revert(map);
  redo_.push_back(std::move(command));
  return true;
}

bool EditHistory::redo(MapData& map) {
  if (redo_.empty()) {
    return false;
  }
  std::unique_ptr<EditCommand> command = std::move(redo_.back());
  redo_.pop_back();
  command->apply(map);
  undo_.push_back(std::move(command));
  return true;
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
