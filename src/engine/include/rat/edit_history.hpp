#pragma once

#include "rat/map_data.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace rat {

class EditCommand {
 public:
  virtual ~EditCommand() = default;
  virtual void apply(MapData& map) = 0;
  virtual void revert(MapData& map) = 0;
};

[[nodiscard]] std::unique_ptr<EditCommand> make_place_blocker_command(BlockerDef blocker);
[[nodiscard]] std::unique_ptr<EditCommand> make_delete_blocker_command(std::size_t index);
[[nodiscard]] std::unique_ptr<EditCommand> make_move_blocker_command(std::size_t index, int tile_dx,
                                                                    int tile_dz, float tile_size);

[[nodiscard]] std::unique_ptr<EditCommand> make_place_event_command(EventDef event);
[[nodiscard]] std::unique_ptr<EditCommand> make_delete_event_command(std::size_t index);
[[nodiscard]] std::unique_ptr<EditCommand> make_move_event_command(std::size_t index, int tile_dx,
                                                                  int tile_dz, float tile_size);

class EditHistory {
 public:
  void execute(MapData& map, std::unique_ptr<EditCommand> command);
  bool undo(MapData& map);
  bool redo(MapData& map);
  void clear();
  [[nodiscard]] bool can_undo() const;
  [[nodiscard]] bool can_redo() const;

 private:
  std::vector<std::unique_ptr<EditCommand>> undo_;
  std::vector<std::unique_ptr<EditCommand>> redo_;
};

}  // namespace rat
