#pragma once

#include "rat/camera.hpp"
#include "rat/event_runtime.hpp"
#include "rat/game_state.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"
#include "rat/surface_query.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

struct HotApplyOptions {
  bool preserve_player_position = true;
};

struct HotApplyTargets {
  EventRuntime& events;
  GameState& state;
  PlayerBody& player;
  std::vector<BlockerDef>& blockers;
  std::vector<Vec3>& event_markers;
  std::unique_ptr<SurfaceQuery>* cached_surface_query = nullptr;
  JumpState* jump_state = nullptr;
};

struct HotApplyResult {
  bool ok = false;
  std::string error;
};

[[nodiscard]] std::vector<Vec3> event_markers_from_map(const MapData& map);

[[nodiscard]] HotApplyResult hot_apply_map(const MapData& map, HotApplyTargets& targets,
                                           HotApplyOptions options = {});

[[nodiscard]] HotApplyResult hot_apply_map_from_string(std::string_view json_text,
                                                       HotApplyTargets& targets,
                                                       HotApplyOptions options = {});

[[nodiscard]] HotApplyResult hot_apply_map_from_file(const std::string& path,
                                                     HotApplyTargets& targets,
                                                     HotApplyOptions options = {});

}  // namespace rat
