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
