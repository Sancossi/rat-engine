#pragma once

#include "rat/map_data.hpp"

#include <string>
#include <string_view>

namespace rat {

struct MapLoadResult {
  bool ok = false;
  std::string error;
  MapData map;
};

struct MapSerializeResult {
  bool ok = false;
  std::string error;
  std::string json_text;
};

struct MapFileResult {
  bool ok = false;
  std::string error;
};

[[nodiscard]] MapLoadResult load_map_from_string(std::string_view json_text);
[[nodiscard]] MapLoadResult load_map_from_file(const std::string& path);
[[nodiscard]] MapSerializeResult serialize_map_to_string(const MapData& map);
[[nodiscard]] MapFileResult save_map_to_file(const MapData& map, const std::string& path);

}  // namespace rat
