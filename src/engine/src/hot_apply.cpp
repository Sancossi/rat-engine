#include "rat/hot_apply.hpp"

#include "rat/map_loader.hpp"

namespace rat {

std::vector<Vec3> event_markers_from_map(const MapData& map) {
  std::vector<Vec3> markers;
  markers.reserve(map.events.size());
  for (const EventDef& event : map.events) {
    if (event.tile.has_value()) {
      markers.push_back({static_cast<float>(event.tile->x) * map.tile_size, 0.0f,
                         static_cast<float>(event.tile->z) * map.tile_size});
    } else if (event.volume.has_value()) {
      markers.push_back({(event.volume->min_x + event.volume->max_x) * 0.5f, 0.0f,
                         (event.volume->min_z + event.volume->max_z) * 0.5f});
    }
  }
  return markers;
}

HotApplyResult hot_apply_map(const MapData& map, HotApplyTargets& targets,
                             HotApplyOptions options) {
  const float keep_x = targets.player.x;
  const float keep_y = targets.player.y;
  const float keep_z = targets.player.z;

  targets.events.load(map);
  targets.state.set_map_id(map.id);
  targets.blockers = map.blockers;
  targets.event_markers = event_markers_from_map(map);

  if (options.preserve_player_position) {
    targets.player.x = keep_x;
    targets.player.y = keep_y;
    targets.player.z = keep_z;
  } else {
    targets.player.x = 0.0f;
    targets.player.y = 0.0f;
    targets.player.z = 0.0f;
  }
  targets.state.set_player_position(targets.player.x, targets.player.y, targets.player.z);

  return HotApplyResult{true, {}};
}

HotApplyResult hot_apply_map_from_string(std::string_view json_text, HotApplyTargets& targets,
                                         HotApplyOptions options) {
  const MapLoadResult loaded = load_map_from_string(json_text);
  if (!loaded.ok) {
    return HotApplyResult{false, loaded.error.empty() ? "map parse failed" : loaded.error};
  }
  return hot_apply_map(loaded.map, targets, options);
}

HotApplyResult hot_apply_map_from_file(const std::string& path, HotApplyTargets& targets,
                                       HotApplyOptions options) {
  const MapLoadResult loaded = load_map_from_file(path);
  if (!loaded.ok) {
    return HotApplyResult{false, loaded.error.empty() ? "map file load failed" : loaded.error};
  }
  return hot_apply_map(loaded.map, targets, options);
}

}  // namespace rat
