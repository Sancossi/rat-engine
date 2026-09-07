#include "rat/retained_memory.hpp"
#include <cstdint>

namespace rat {
std::size_t retained_dynamic_bytes(const std::string& value) {
  // Do not count a standard library's inline small-string buffer twice.
  const auto data = reinterpret_cast<std::uintptr_t>(value.data());
  const auto object = reinterpret_cast<std::uintptr_t>(&value);
  return data >= object && data - object < sizeof(value) ? 0 : value.capacity() + 1;
}
namespace {
std::size_t command_bytes(const Command& v) {
  auto n = retained_dynamic_bytes(v.text) + retained_dynamic_bytes(v.map_id)
      + retained_dynamic_bytes(v.item_id) + retained_dynamic_bytes(v.branch_condition.string_id)
      + retained_vector_bytes(v.route) + retained_vector_bytes(v.then_commands)
      + retained_vector_bytes(v.else_commands);
  for (const auto& child : v.then_commands) n += command_bytes(child);
  for (const auto& child : v.else_commands) n += command_bytes(child);
  return n;
}
std::size_t graph_bytes(const EventGraph& v) {
  auto n = retained_vector_bytes(v.nodes) + retained_vector_bytes(v.edges);
  for (const auto& node : v.nodes)
    n += retained_dynamic_bytes(node.id) + retained_dynamic_bytes(node.kind)
        + retained_dynamic_bytes(node.text) + retained_dynamic_bytes(node.map_id)
        + retained_dynamic_bytes(node.item_id) + retained_dynamic_bytes(node.branch_condition.string_id)
        + retained_vector_bytes(node.route);
  for (const auto& edge : v.edges)
    n += retained_dynamic_bytes(edge.from) + retained_dynamic_bytes(edge.to)
        + (edge.branch ? retained_dynamic_bytes(*edge.branch) : 0);
  return n;
}
}  // namespace
std::size_t retained_dynamic_bytes(const EventDef& v) {
  auto n = retained_dynamic_bytes(v.id) + retained_vector_bytes(v.pages);
  for (const auto& page : v.pages) {
    n += retained_vector_bytes(page.conditions) + retained_vector_bytes(page.commands);
    for (const auto& condition : page.conditions) n += retained_dynamic_bytes(condition.string_id);
    for (const auto& command : page.commands) n += command_bytes(command);
    if (page.graph) n += graph_bytes(*page.graph);
  }
  return n;
}
std::size_t retained_dynamic_bytes(const MapData& v) {
  auto n = retained_dynamic_bytes(v.id) + retained_vector_bytes(v.height_grid.ground_y)
      + retained_vector_bytes(v.ramps) + retained_vector_bytes(v.edge_barriers)
      + retained_vector_bytes(v.floor_slabs) + retained_vector_bytes(v.ladders)
      + retained_vector_bytes(v.indoor_volumes) + retained_vector_bytes(v.occupancy)
      + retained_vector_bytes(v.blockers) + retained_vector_bytes(v.events)
      + retained_vector_bytes(v.assets);
  for (const auto& event : v.events) n += retained_dynamic_bytes(event);
  for (const auto& asset : v.assets)
    n += retained_dynamic_bytes(asset.id.catalog_key) + retained_dynamic_bytes(asset.debug_name);
  return n;
}
}  // namespace rat
