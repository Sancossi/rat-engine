#pragma once

#include "rat/map_data.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace rat {

struct EditApplyResult {
  bool applied = false;
  bool mutates_blockers = false;
  bool mutates_events = false;
  bool mutates_elevation = false;
  std::string error;

  explicit operator bool() const { return applied; }
};

class EditCommand {
 public:
  virtual ~EditCommand() = default;
  virtual void apply(MapData& map) = 0;
  virtual void revert(MapData& map) = 0;
  [[nodiscard]] virtual bool applied_successfully() const { return true; }
  [[nodiscard]] virtual std::string last_error() const { return {}; }
  [[nodiscard]] virtual bool mutates_blockers() const = 0;
  [[nodiscard]] virtual bool mutates_events() const = 0;
  [[nodiscard]] virtual bool mutates_elevation() const { return false; }
};

[[nodiscard]] std::unique_ptr<EditCommand> make_place_blocker_command(BlockerDef blocker);
[[nodiscard]] std::unique_ptr<EditCommand> make_delete_blocker_command(std::size_t index);
[[nodiscard]] std::unique_ptr<EditCommand> make_move_blocker_command(std::size_t index, int tile_dx,
                                                                    int tile_dz, float tile_size);
[[nodiscard]] std::unique_ptr<EditCommand> make_replace_blocker_command(std::size_t index,
                                                                        BlockerDef next);

[[nodiscard]] std::unique_ptr<EditCommand> make_place_event_command(EventDef event);
[[nodiscard]] std::unique_ptr<EditCommand> make_delete_event_command(std::size_t index);
[[nodiscard]] std::unique_ptr<EditCommand> make_move_event_command(std::size_t index, int tile_dx,
                                                                  int tile_dz, float tile_size);
[[nodiscard]] std::unique_ptr<EditCommand> make_replace_event_command(std::size_t index,
                                                                      EventDef next);
[[nodiscard]] std::unique_ptr<EditCommand> make_set_map_tile_ground_y_command(
    int tile_x, int tile_z, float ground_y);
[[nodiscard]] std::unique_ptr<EditCommand> make_adjust_map_tile_ground_y_command(
    int tile_x, int tile_z, float delta_y);
[[nodiscard]] std::unique_ptr<EditCommand> make_place_map_tile_cube_command(int tile_x, int tile_z);
[[nodiscard]] std::unique_ptr<EditCommand> make_upsert_map_ramp_command(RampDef ramp);
[[nodiscard]] std::unique_ptr<EditCommand> make_remove_map_ramp_command(TileCoord tile);
[[nodiscard]] std::unique_ptr<EditCommand> make_upsert_map_edge_barrier_command(
    EdgeBarrierDef edge);
[[nodiscard]] std::unique_ptr<EditCommand> make_remove_map_edge_barrier_command(
    TileCoord tile, RampDirection direction);

class EditHistory {
 public:
  EditApplyResult execute(MapData& map, std::unique_ptr<EditCommand> command);
  EditApplyResult undo(MapData& map);
  EditApplyResult redo(MapData& map);
  void clear();
  [[nodiscard]] bool can_undo() const;
  [[nodiscard]] bool can_redo() const;

 private:
  std::vector<std::unique_ptr<EditCommand>> undo_;
  std::vector<std::unique_ptr<EditCommand>> redo_;
};

}  // namespace rat
