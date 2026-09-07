#pragma once

#include "rat/map_data.hpp"
#include <cstddef>

namespace rat {
// Estimated owned allocation bytes, excluding the containing object's sizeof,
// allocator metadata/padding and opaque callable allocations. Vector capacity
// includes unused elements; nested allocations are visited only for live objects.
[[nodiscard]] std::size_t retained_dynamic_bytes(const std::string& value);
[[nodiscard]] std::size_t retained_dynamic_bytes(const EventDef& value);
[[nodiscard]] std::size_t retained_dynamic_bytes(const MapData& value);
template<class T> [[nodiscard]] std::size_t retained_vector_bytes(const std::vector<T>& value) {
  return value.capacity() * sizeof(T);
}
}  // namespace rat
