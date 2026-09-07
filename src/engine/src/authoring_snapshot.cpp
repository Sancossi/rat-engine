#include "rat/authoring_snapshot.hpp"

#include <bit>
#include <cstdint>
#include <type_traits>

namespace rat {
namespace {
struct Snapshot {
  std::string bytes;
  template <class T> requires (std::is_integral_v<T> || std::is_enum_v<T>)
  void put(T value) {
    const auto bits = static_cast<std::uint64_t>(value);
    for (int i = 0; i < 8; ++i) bytes.push_back(static_cast<char>(bits >> (i * 8)));
  }
  void put(float value) { put(std::bit_cast<std::uint32_t>(value == 0.0f ? 0.0f : value)); }
  void put(const std::string& value) { put(value.size()); bytes.append(value); }
  template <class T> void put(const std::optional<T>& value) {
    put(value.has_value()); if (value) put(*value);
  }
  template <class T> void put(const std::vector<T>& values) {
    put(values.size()); for (const auto& value : values) put(value);
  }
  template <class... T> void fields(const T&... values) { (put(values), ...); }
  void put(const TileCoord& v) { fields(v.x, v.z); }
  void put(const Aabb2& v) { fields(v.min_x, v.min_z, v.max_x, v.max_z); }
  void put(const HeightGrid& v) { fields(v.origin_x, v.origin_z, v.width, v.height, v.ground_y); }
  void put(const RampDef& v) { fields(v.tile, v.direction, v.low_y, v.high_y); }
  void put(const EdgeBarrierDef& v) { fields(v.tile, v.direction, v.height); }
  void put(const FloorSlabDef& v) { fields(v.tile, v.top_y, v.thickness); }
  void put(const LadderDef& v) { fields(v.tile, v.direction, v.y_lo, v.y_hi); }
  void put(const IndoorVolume& v) { fields(v.xz, v.y_lo, v.y_hi); }
  void put(const OccupancyCell& v) { fields(v.x, v.y, v.z, v.kind, v.yaw); }
  void put(const BlockerDef& v) { fields(v.bounds, v.base_y, v.top_y, v.jumpable); }
  void put(const Condition& v) { fields(v.type, v.id, v.bool_value, v.int_value, v.op, v.string_id, v.self_switch); }
  void put(const RouteStep& v) { fields(v.op, v.dir, v.frames); }
  void put(const Command& v) {
    fields(v.op, v.text, v.id, v.bool_value, v.int_value, v.frames, v.map_id,
      v.x, v.y, v.z, v.item_id, v.item_delta, v.key_item, v.self_switch,
      v.branch_condition, v.then_commands, v.else_commands, v.through, v.route);
  }
  void put(const EventGraphNodeLayout& v) { fields(v.x, v.y); }
  void put(const EventGraphNode& v) {
    fields(v.id, v.kind, v.text, v.switch_id, v.bool_value, v.frames,
      v.branch_condition, v.int_value, v.self_switch, v.map_id, v.x, v.y, v.z,
      v.item_id, v.item_delta, v.key_item, v.through, v.route, v.layout);
  }
  void put(const EventGraphEdge& v) { fields(v.from, v.to, v.order, v.branch); }
  void put(const EventGraph& v) { fields(v.nodes, v.edges); }
  void put(const EventPage& v) {
    fields(v.trigger, v.conditions, v.graph);
    if (!v.graph) put(v.commands);
  }
  void put(const EventDef& v) { fields(v.id, v.tile, v.volume, v.y, v.pages); }
  void put(const MapAssetRef& v) { fields(v.id.key(), v.kind, v.debug_name); }
};
}

std::string authoring_snapshot(const MapData& map) {
  Snapshot s;
  s.fields(map.schema_version, map.id, map.width, map.height, map.tile_size,
    map.height_grid, map.ramps, map.edge_barriers, map.floor_slabs, map.ladders,
    map.indoor_volumes, map.occupancy, map.blockers, map.events, map.assets);
  return std::move(s.bytes);
}
}
