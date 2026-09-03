#include "rat/event_edit.hpp"

#include "rat/blocker_edit.hpp"

namespace rat {

Vec3 tile_center_world(TileCoord tile, float tile_size) {
  return {(static_cast<float>(tile.x) + 0.5f) * tile_size, 0.0f,
          (static_cast<float>(tile.z) + 0.5f) * tile_size};
}

EventDef make_stub_event(std::string id, int tile_x, int tile_z) {
  EventDef event;
  event.id = std::move(id);
  event.tile = TileCoord{tile_x, tile_z};
  EventPage page;
  page.trigger = TriggerKind::Action;
  Command text;
  text.op = CommandOp::ShowText;
  text.text = "New event";
  page.commands.push_back(std::move(text));
  event.pages.push_back(std::move(page));
  return event;
}

namespace {

[[nodiscard]] bool event_id_taken(const MapData& map, std::string_view id) {
  for (const EventDef& event : map.events) {
    if (event.id == id) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool tile_occupied_by_event(const MapData& map, TileCoord tile) {
  for (const EventDef& event : map.events) {
    if (event.tile.has_value() && event.tile->x == tile.x && event.tile->z == tile.z) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::string allocate_unique_event_id(const MapData& map, std::string_view base) {
  if (!event_id_taken(map, base)) {
    return std::string(base);
  }
  const std::string copy = std::string(base) + "_copy";
  if (!event_id_taken(map, copy)) {
    return copy;
  }
  for (int n = 2;; ++n) {
    const std::string candidate = std::string(base) + "_copy" + std::to_string(n);
    if (!event_id_taken(map, candidate)) {
      return candidate;
    }
  }
}

EventDef make_duplicate_event(const MapData& map, std::size_t index) {
  EventDef copy = map.events[index];
  copy.id = allocate_unique_event_id(map, copy.id);
  if (!copy.tile.has_value()) {
    return copy;
  }
  int dx = 1;
  TileCoord candidate{copy.tile->x + dx, copy.tile->z};
  while (tile_occupied_by_event(map, candidate)) {
    ++dx;
    candidate.x = copy.tile->x + dx;
  }
  const float tile_size = map.tile_size > 0.0f ? map.tile_size : 1.0f;
  translate_event_on_grid(copy, dx, 0, tile_size);
  return copy;
}

void translate_event_on_grid(EventDef& event, int tile_dx, int tile_dz, float tile_size) {
  if (event.tile.has_value()) {
    event.tile->x += tile_dx;
    event.tile->z += tile_dz;
  }
  if (event.volume.has_value()) {
    event.volume = translate_aabb_on_grid(*event.volume, tile_dx, tile_dz, tile_size);
  }
}

int event_marker_index(const MapData& map, int event_index) {
  if (event_index < 0 || event_index >= static_cast<int>(map.events.size())) {
    return -1;
  }
  int marker = 0;
  for (int i = 0; i < static_cast<int>(map.events.size()); ++i) {
    const EventDef& event = map.events[static_cast<std::size_t>(i)];
    if (!event.tile.has_value() && !event.volume.has_value()) {
      continue;
    }
    if (i == event_index) {
      return marker;
    }
    ++marker;
  }
  return -1;
}

}  // namespace rat
