#include "rat/hot_apply.hpp"

#include "rat/event_edit.hpp"
#include "rat/map_loader.hpp"

namespace rat {
namespace {

void reset_jump_state(JumpState& jump) {
  jump = make_grounded_jump_state();
}

}  // namespace

std::vector<Vec3> event_markers_from_map(const MapData& map) {
  const SurfaceQuery query(map);
  std::vector<Vec3> markers;
  markers.reserve(map.events.size());
  for (const EventDef& event : map.events) {
    if (event.tile.has_value()) {
      Vec3 marker = tile_center_world(*event.tile, map.tile_size);
      marker.y = query.sample(marker.x, marker.z).y;
      markers.push_back(marker);
    } else if (event.volume.has_value()) {
      Vec3 marker{
          (event.volume->min_x + event.volume->max_x) * 0.5f,
          0.0f,
          (event.volume->min_z + event.volume->max_z) * 0.5f,
      };
      marker.y = query.sample(marker.x, marker.z).y;
      markers.push_back(marker);
    }
  }
  return markers;
}

HotApplyResult hot_apply_map(const MapData& map, HotApplyTargets& targets,
                             HotApplyOptions options) {
  if (targets.cached_surface_query != nullptr) {
    if (*targets.cached_surface_query == nullptr) {
      *targets.cached_surface_query = std::make_unique<SurfaceQuery>(map);
    } else {
      **targets.cached_surface_query = SurfaceQuery(map);
    }
  }

  SurfaceQuery local_query(map);
  const SurfaceQuery* query = &local_query;
  if (targets.cached_surface_query != nullptr && *targets.cached_surface_query != nullptr) {
    query = targets.cached_surface_query->get();
  }

  const float keep_x = targets.player.x;
  const float keep_z = targets.player.z;

  targets.events.load(map);
  targets.state.set_map_id(map.id);
  targets.blockers = map.blockers;
  targets.event_markers = event_markers_from_map(map);

  if (options.preserve_player_position) {
    targets.player.x = keep_x;
    targets.player.z = keep_z;
  } else {
    targets.player.x = 0.0f;
    targets.player.z = 0.0f;
  }
  targets.player.y = query->sample(targets.player.x, targets.player.z).y;
  targets.state.set_player_position(targets.player.x, targets.player.y, targets.player.z);
  if (targets.jump_state != nullptr) {
    reset_jump_state(*targets.jump_state);
  }

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
