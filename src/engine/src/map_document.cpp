#include "rat/map_document.hpp"

#include "rat/event_graph.hpp"
#include "rat/map_loader.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cmath>
#include <limits>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace rat {
namespace {

using json = nlohmann::json;

struct TileKey {
  int x = 0;
  int z = 0;

  [[nodiscard]] bool operator==(const TileKey& other) const {
    return x == other.x && z == other.z;
  }
};

struct TileKeyHash {
  [[nodiscard]] std::size_t operator()(const TileKey& key) const {
    const auto hx = std::hash<int>{}(key.x);
    const auto hz = std::hash<int>{}(key.z);
    return hx ^ (hz + 0x9e3779b9 + (hx << 6) + (hx >> 2));
  }
};

struct EdgeKey {
  int x = 0;
  int z = 0;
  RampDirection direction = RampDirection::East;

  [[nodiscard]] bool operator==(const EdgeKey& other) const {
    return x == other.x && z == other.z && direction == other.direction;
  }
};

struct EdgeKeyHash {
  [[nodiscard]] std::size_t operator()(const EdgeKey& key) const {
    const auto hx = std::hash<int>{}(key.x);
    const auto hz = std::hash<int>{}(key.z);
    const auto hd = std::hash<int>{}(static_cast<int>(key.direction));
    std::size_t h = hx;
    h ^= hz + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hd + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

[[nodiscard]] std::string index_path(std::string_view prefix, std::size_t index) {
  std::string path;
  path.reserve(prefix.size() + 8);
  path.append(prefix);
  path.push_back('/');
  path.append(std::to_string(index));
  return path;
}

void add_error(std::vector<MapIssue>& issues, std::string json_path, std::string message) {
  MapIssue issue;
  issue.severity = MapIssueSeverity::Error;
  issue.json_path = std::move(json_path);
  issue.message = std::move(message);
  issues.push_back(std::move(issue));
}

[[nodiscard]] bool valid_ramp_direction(RampDirection direction) {
  switch (direction) {
    case RampDirection::North:
    case RampDirection::East:
    case RampDirection::South:
    case RampDirection::West:
      return true;
  }
  return false;
}

[[nodiscard]] bool tile_in_grid(const HeightGrid& grid, int tile_x, int tile_z) {
  if (grid.width <= 0 || grid.height <= 0) {
    return false;
  }
  const auto local_x = static_cast<std::int64_t>(tile_x) - grid.origin_x;
  const auto local_z = static_cast<std::int64_t>(tile_z) - grid.origin_z;
  return local_x >= 0 && local_z >= 0 && local_x < grid.width && local_z < grid.height;
}

[[nodiscard]] bool valid_self_switch(char key) {
  return key >= 'A' && key <= 'D';
}

void apply_v1_height_fallback(MapData& map) {
  if (map.schema_version != 1) {
    return;
  }
  const bool grid_missing = map.height_grid.width <= 0 || map.height_grid.height <= 0;
  if (!grid_missing || !safe_map_grid(map.width, map.height)) {
    return;
  }
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = map.width;
  map.height_grid.height = map.height;
  map.height_grid.ground_y.assign(static_cast<std::size_t>(map.width) *
                                      static_cast<std::size_t>(map.height),
                                  0.0f);
}

void validate_condition(const Condition& condition, const std::string& path,
                        std::vector<MapIssue>& issues) {
  if (condition.type == ConditionType::SelfSwitch && !valid_self_switch(condition.self_switch)) {
    add_error(issues, path, "self_switch key must be A-D");
  }
  if (condition.type == ConditionType::Item && condition.string_id.empty()) {
    add_error(issues, path, "item id must not be empty");
  }
}

void validate_commands(const std::vector<Command>& commands, const std::string& path,
                       std::vector<MapIssue>& issues);

void validate_command(const Command& command, const std::string& path,
                      std::vector<MapIssue>& issues) {
  switch (command.op) {
    case CommandOp::PlaySE:
      if (command.text.empty()) {
        add_error(issues, path, "play_se id must not be empty");
      }
      break;
    case CommandOp::SetMoveRoute:
      for (const RouteStep& step : command.route) {
        if (step.op == RouteStepOp::Wait && step.frames < 0) {
          add_error(issues, path, "set_move_route wait frames must be >= 0");
        }
      }
      break;
    case CommandOp::Wait:
      if (command.frames < 0) {
        add_error(issues, path, "wait frames must be >= 0");
      }
      break;
    case CommandOp::TransferPlayer:
      if (command.map_id.empty()) {
        add_error(issues, path, "transfer_player map_id must not be empty");
      }
      break;
    case CommandOp::ChangeItems:
      if (command.item_id.empty()) {
        add_error(issues, path, "change_items id must not be empty");
      }
      break;
    case CommandOp::ControlSelfSwitch:
      if (!valid_self_switch(command.self_switch)) {
        add_error(issues, path, "control_self_switch key must be A-D");
      }
      break;
    case CommandOp::ConditionalBranch:
      validate_condition(command.branch_condition, path + "/condition", issues);
      validate_commands(command.then_commands, path + "/then", issues);
      validate_commands(command.else_commands, path + "/else", issues);
      break;
    case CommandOp::ShowText:
    case CommandOp::ControlSwitch:
    case CommandOp::ControlVariable:
    case CommandOp::Comment:
      break;
  }
}

void validate_commands(const std::vector<Command>& commands, const std::string& path,
                       std::vector<MapIssue>& issues) {
  for (std::size_t i = 0; i < commands.size(); ++i) {
    validate_command(commands[i], index_path(path, i), issues);
  }
}

[[nodiscard]] std::string guess_json_path_from_loader_error(std::string_view message) {
  if (message.find("/occupancy/") != std::string_view::npos) {
    const auto start = message.find("/occupancy/");
    auto end = start;
    while (end < message.size() && message[end] != ' ' && message[end] != ':') {
      ++end;
    }
    return std::string(message.substr(start, end - start));
  }
  if (message.find("schema_version") != std::string_view::npos) {
    return "/schema_version";
  }
  if (message.find("height_grid ground_y") != std::string_view::npos) {
    return "/height_grid/ground_y";
  }
  if (message.find("height_grid") != std::string_view::npos) {
    return "/height_grid";
  }
  if (message.find("map id") != std::string_view::npos) {
    return "/id";
  }
  if (message.find("width/height") != std::string_view::npos) {
    return "/width";
  }
  if (message.find("ramp") != std::string_view::npos) {
    return "/ramps";
  }
  return "/";
}

}  // namespace

MapDocument::MapDocument(MapData data) : data_(std::move(data)), revision_(1) {}

void MapDocument::replace(MapData data) {
  data_ = std::move(data);
  ++revision_;
}

bool map_issues_have_errors(const std::vector<MapIssue>& issues) {
  for (const MapIssue& issue : issues) {
    if (issue.severity == MapIssueSeverity::Error) {
      return true;
    }
  }
  return false;
}

std::string format_map_issues(const std::vector<MapIssue>& issues) {
  std::ostringstream oss;
  bool first = true;
  for (const MapIssue& issue : issues) {
    if (!first) {
      oss << "; ";
    }
    first = false;
    oss << issue.json_path << " ["
        << (issue.severity == MapIssueSeverity::Error ? "error" : "warning") << "]: "
        << issue.message;
  }
  return oss.str();
}

bool safe_map_grid(int width, int height, int origin_x, int origin_z) {
  if (width <= 0 || height <= 0 ||
      static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > kMaxMapGridCells)
    return false;
  // Keep integer neighbor/corner arithmetic representable as well.
  const auto safe_axis = [](int origin, int size) {
    return origin > (std::numeric_limits<int>::min)() &&
           static_cast<std::int64_t>(origin) + size < (std::numeric_limits<int>::max)();
  };
  return safe_axis(origin_x, width) && safe_axis(origin_z, height);
}

std::vector<MapIssue> validate_map_structure(const MapData& data) {
  std::vector<MapIssue> issues;
  const auto finite = [&](float value, const std::string& path) {
    if (!std::isfinite(value)) add_error(issues, path, "number must be finite");
  };
  const auto bounds = [&](const Aabb2& box, const std::string& path) {
    finite(box.min_x, path + "/min_x"); finite(box.min_z, path + "/min_z");
    finite(box.max_x, path + "/max_x"); finite(box.max_z, path + "/max_z");
    finite(box.max_x - box.min_x, path + "/width");
    finite(box.max_z - box.min_z, path + "/depth");
    finite(box.max_x + box.min_x, path + "/center_x");
    finite(box.max_z + box.min_z, path + "/center_z");
  };
  const auto world_tile = [&](TileCoord tile, const std::string& path) {
    finite(static_cast<float>(tile.x) * data.tile_size, path + "/x");
    finite(static_cast<float>(static_cast<double>(tile.x) + 1.0) * data.tile_size, path + "/x");
    finite(static_cast<float>(tile.z) * data.tile_size, path + "/z");
    finite(static_cast<float>(static_cast<double>(tile.z) + 1.0) * data.tile_size, path + "/z");
  };
  if (data.id.empty()) add_error(issues, "/id", "map id must not be empty");
  if (data.schema_version < 1 || data.schema_version > 5)
    add_error(issues, "/schema_version", "unsupported schema_version");
  if (!safe_map_grid(data.width, data.height))
    add_error(issues, "/width", "map width/height exceed safe grid limits");
  finite(data.tile_size, "/tile_size");
  if (data.tile_size <= 0.0f) add_error(issues, "/tile_size", "tile_size must be > 0");
  const auto& grid = data.height_grid;
  const bool v1_missing = data.schema_version == 1 && (grid.width <= 0 || grid.height <= 0);
  if (!v1_missing) {
    if (!safe_map_grid(grid.width, grid.height, grid.origin_x, grid.origin_z))
      add_error(issues, "/height_grid/width", "height_grid dimensions exceed safe grid limits");
    else if (grid.ground_y.size() != static_cast<std::size_t>(grid.width) * grid.height)
      add_error(issues, "/height_grid/ground_y", "height_grid ground_y length mismatch");
  }
  for (std::size_t i = 0; i < grid.ground_y.size(); ++i)
    finite(grid.ground_y[i], index_path("/height_grid/ground_y", i));
  // The derived world extents must also be representable as float.
  for (double coordinate : {static_cast<double>(grid.origin_x), static_cast<double>(grid.origin_z),
                             static_cast<double>(grid.origin_x) + grid.width,
                             static_cast<double>(grid.origin_z) + grid.height,
                             static_cast<double>(data.width), static_cast<double>(data.height)}) {
    if (std::abs(coordinate * data.tile_size) > (std::numeric_limits<float>::max)())
      add_error(issues, "/tile_size", "world extent exceeds float range");
  }
  for (std::size_t i = 0; i < data.ramps.size(); ++i) {
    const auto path = index_path("/ramps", i);
    finite(data.ramps[i].low_y, path + "/low_y"); finite(data.ramps[i].high_y, path + "/high_y");
    finite(data.ramps[i].high_y - data.ramps[i].low_y, path + "/rise");
    world_tile(data.ramps[i].tile, path + "/tile");
  }
  for (std::size_t i = 0; i < data.edge_barriers.size(); ++i) {
    const auto& edge = data.edge_barriers[i];
    const auto path = index_path("/edge_barriers", i);
    finite(edge.height, path + "/height");
    world_tile(edge.tile, path + "/tile");
    if (tile_in_grid(grid, edge.tile.x, edge.tile.z)) {
      const auto x = static_cast<std::int64_t>(edge.tile.x) - grid.origin_x;
      const auto z = static_cast<std::int64_t>(edge.tile.z) - grid.origin_z;
      const auto index = static_cast<std::size_t>(z) * grid.width + static_cast<std::size_t>(x);
      if (index < grid.ground_y.size()) finite(grid.ground_y[index] + edge.height, path + "/top_y");
      for (const auto& ramp : data.ramps) {
        if (ramp.tile.x == edge.tile.x && ramp.tile.z == edge.tile.z) {
          finite(ramp.low_y + edge.height, path + "/top_y");
          finite(ramp.high_y + edge.height, path + "/top_y");
        }
      }
    }
  }
  for (std::size_t i = 0; i < data.floor_slabs.size(); ++i) {
    const auto path = index_path("/floor_slabs", i);
    finite(data.floor_slabs[i].top_y, path + "/top_y");
    finite(data.floor_slabs[i].thickness, path + "/thickness");
    finite(data.floor_slabs[i].top_y - data.floor_slabs[i].thickness, path);
    world_tile(data.floor_slabs[i].tile, path + "/tile");
  }
  for (std::size_t i = 0; i < data.ladders.size(); ++i) {
    const auto path = index_path("/ladders", i);
    finite(data.ladders[i].y_lo, path + "/y_lo"); finite(data.ladders[i].y_hi, path + "/y_hi");
    finite(data.ladders[i].y_hi - data.ladders[i].y_lo, path + "/height");
    world_tile(data.ladders[i].tile, path + "/tile");
  }
  for (std::size_t i = 0; i < data.indoor_volumes.size(); ++i) {
    const auto path = index_path("/indoor_volumes", i);
    bounds(data.indoor_volumes[i].xz, path);
    finite(data.indoor_volumes[i].y_lo, path + "/y_lo"); finite(data.indoor_volumes[i].y_hi, path + "/y_hi");
    finite(data.indoor_volumes[i].y_hi - data.indoor_volumes[i].y_lo, path + "/height");
  }
  for (std::size_t i = 0; i < data.blockers.size(); ++i) {
    const auto path = index_path("/blockers", i);
    bounds(data.blockers[i].bounds, path);
    if (data.blockers[i].base_y) finite(*data.blockers[i].base_y, path + "/base_y");
    if (data.blockers[i].top_y) finite(*data.blockers[i].top_y, path + "/top_y");
    if (data.blockers[i].base_y && data.blockers[i].top_y)
      finite(*data.blockers[i].top_y - *data.blockers[i].base_y, path + "/height");
  }
  for (std::size_t i = 0; i < data.occupancy.size(); ++i) {
    const auto& cell = data.occupancy[i];
    const auto path = index_path("/occupancy", i);
    world_tile({cell.x, cell.z}, path);
    const float bottom = static_cast<float>(cell.y) * data.tile_size;
    const float top = static_cast<float>(static_cast<double>(cell.y) + 1.0) * data.tile_size;
    finite(bottom, path + "/bottom_y");
    finite(top, path + "/top_y");
    finite(top - data.tile_size, path + "/slab_bottom_y");
    if (!tile_in_grid(grid, cell.x, cell.z)) add_error(issues, path, "occupancy cell xz is outside height grid");
    if (cell.y == (std::numeric_limits<int>::max)() || cell.y == (std::numeric_limits<int>::min)())
      add_error(issues, path + "/y", "occupancy neighbor height exceeds integer range");
  }
  std::function<void(const std::vector<Command>&, const std::string&)> commands;
  commands = [&](const std::vector<Command>& list, const std::string& path) {
    for (std::size_t i = 0; i < list.size(); ++i) {
      const auto item_path = index_path(path, i);
      finite(list[i].x, item_path + "/x"); finite(list[i].y, item_path + "/y"); finite(list[i].z, item_path + "/z");
      commands(list[i].then_commands, item_path + "/then");
      commands(list[i].else_commands, item_path + "/else");
    }
  };
  std::unordered_set<std::string> event_ids;
  for (std::size_t i = 0; i < data.events.size(); ++i) {
    const auto& event = data.events[i];
    const auto path = index_path("/events", i);
    if (event.id.empty() || !event_ids.insert(event.id).second)
      add_error(issues, path + "/id", "event id must be nonempty and unique");
    if (event.tile) world_tile(*event.tile, path + "/tile");
    if (event.volume) bounds(*event.volume, path + "/volume");
    if (event.y) finite(*event.y, path + "/y");
    for (std::size_t p = 0; p < event.pages.size(); ++p) {
      const auto page_path = index_path(path + "/pages", p);
      const auto& page = event.pages[p];
      commands(page.commands, page_path + "/commands");
      if (page.graph) for (std::size_t n = 0; n < page.graph->nodes.size(); ++n) {
        const auto node_path = index_path(page_path + "/graph/nodes", n) + "/params";
        const auto& node = page.graph->nodes[n];
        if (node.layout) {
          finite(node.layout->x, index_path(page_path + "/graph/nodes", n) + "/layout/x");
          finite(node.layout->y, index_path(page_path + "/graph/nodes", n) + "/layout/y");
        }
        finite(node.x, node_path + "/x"); finite(node.y, node_path + "/y"); finite(node.z, node_path + "/z");
      }
    }
  }
  std::unordered_set<std::string> asset_ids;
  for (std::size_t i = 0; i < data.assets.size(); ++i)
    if (!data.assets[i].id.valid() || !asset_ids.insert(data.assets[i].id.key()).second)
      add_error(issues, index_path("/assets", i) + "/id", "asset id must be nonempty and unique");
  return issues;
}

std::vector<MapIssue> validate_map_document(const MapData& data) {
  std::vector<MapIssue> issues = validate_map_structure(data);
  if (data.id.empty()) {
    add_error(issues, "/id", "map id must not be empty");
  }
  if (data.schema_version != 1 && data.schema_version != 2 && data.schema_version != 3 &&
      data.schema_version != 4 && data.schema_version != 5) {
    add_error(issues, "/schema_version", "unsupported schema_version (expected 1, 2, 3, 4, or 5)");
  }
  if (data.width <= 0 || data.height <= 0) {
    add_error(issues, "/width", "map width/height must be > 0");
  }
  if (data.tile_size <= 0.0f) {
    add_error(issues, "/tile_size", "tile_size must be > 0");
  }

  if (data.height_grid.width <= 0 || data.height_grid.height <= 0) {
    add_error(issues, "/height_grid/width", "height_grid width/height must be > 0");
  } else {
    const std::size_t expected = static_cast<std::size_t>(data.height_grid.width) *
                                 static_cast<std::size_t>(data.height_grid.height);
    if (data.height_grid.ground_y.size() != expected) {
      add_error(issues, "/height_grid/ground_y", "height_grid ground_y length mismatch");
    }
  }

  std::unordered_set<TileKey, TileKeyHash> ramp_tiles;
  for (std::size_t i = 0; i < data.ramps.size(); ++i) {
    const RampDef& ramp = data.ramps[i];
    const std::string ramp_path = index_path("/ramps", i);
    if (!valid_ramp_direction(ramp.direction)) {
      add_error(issues, ramp_path + "/direction", "ramp direction is invalid");
    }
    if (ramp.high_y < ramp.low_y) {
      add_error(issues, ramp_path + "/high_y", "ramp high_y must be >= low_y");
    }
    if (!tile_in_grid(data.height_grid, ramp.tile.x, ramp.tile.z)) {
      add_error(issues, ramp_path + "/tile", "ramp tile is outside height grid");
    }
    const TileKey key{ramp.tile.x, ramp.tile.z};
    if (!ramp_tiles.insert(key).second) {
      add_error(issues, ramp_path + "/tile", "ramp tile must be unique");
    }
  }

  std::unordered_set<EdgeKey, EdgeKeyHash> edge_keys;
  for (std::size_t i = 0; i < data.edge_barriers.size(); ++i) {
    const EdgeBarrierDef& edge = data.edge_barriers[i];
    const std::string edge_path = index_path("/edge_barriers", i);
    if (!valid_ramp_direction(edge.direction)) {
      add_error(issues, edge_path + "/direction", "edge barrier direction is invalid");
    }
    if (edge.height <= 0.0f) {
      add_error(issues, edge_path + "/height", "edge barrier height must be > 0");
    }
    if (!tile_in_grid(data.height_grid, edge.tile.x, edge.tile.z)) {
      add_error(issues, edge_path + "/tile", "edge barrier tile is outside height grid");
    }
    if (ramp_tiles.find(TileKey{edge.tile.x, edge.tile.z}) != ramp_tiles.end()) {
      add_error(issues, edge_path + "/tile", "edge barrier cannot sit on a ramp tile");
    }
    const EdgeKey key{edge.tile.x, edge.tile.z, edge.direction};
    if (!edge_keys.insert(key).second) {
      add_error(issues, edge_path, "edge barrier tile and direction must be unique");
    }
  }

  for (std::size_t i = 0; i < data.floor_slabs.size(); ++i) {
    const FloorSlabDef& slab = data.floor_slabs[i];
    const std::string slab_path = index_path("/floor_slabs", i);
    if (slab.thickness <= 0.0f) {
      add_error(issues, slab_path + "/thickness", "floor slab thickness must be > 0");
    }
    if (!tile_in_grid(data.height_grid, slab.tile.x, slab.tile.z)) {
      add_error(issues, slab_path + "/tile", "floor slab tile is outside height grid");
    }
    const float a_lo = slab.top_y - slab.thickness;
    const float a_hi = slab.top_y;
    for (std::size_t j = 0; j < i; ++j) {
      const FloorSlabDef& other = data.floor_slabs[j];
      if (other.tile.x != slab.tile.x || other.tile.z != slab.tile.z) {
        continue;
      }
      const float b_lo = other.top_y - other.thickness;
      const float b_hi = other.top_y;
      if (a_lo < b_hi - 1e-4f && b_lo < a_hi - 1e-4f) {
        add_error(issues, slab_path + "/top_y",
                  "floor slab Y range overlaps another slab on the same tile");
        break;
      }
    }
  }

  for (std::size_t i = 0; i < data.ladders.size(); ++i) {
    const LadderDef& ladder = data.ladders[i];
    const std::string ladder_path = index_path("/ladders", i);
    if (!valid_ramp_direction(ladder.direction)) {
      add_error(issues, ladder_path + "/direction", "ladder direction is invalid");
    }
    if (ladder.y_hi <= ladder.y_lo) {
      add_error(issues, ladder_path + "/y_hi", "ladder y_hi must be > y_lo");
    }
    if (!tile_in_grid(data.height_grid, ladder.tile.x, ladder.tile.z)) {
      add_error(issues, ladder_path + "/tile", "ladder tile is outside height grid");
    }
  }

  for (std::size_t i = 0; i < data.indoor_volumes.size(); ++i) {
    const IndoorVolume& volume = data.indoor_volumes[i];
    const std::string volume_path = index_path("/indoor_volumes", i);
    if (volume.y_hi < volume.y_lo) {
      add_error(issues, volume_path + "/y_hi", "indoor volume y_hi must be >= y_lo");
    }
    if (volume.xz.max_x < volume.xz.min_x || volume.xz.max_z < volume.xz.min_z) {
      add_error(issues, volume_path, "indoor volume xz max must be >= min");
    }
  }

  for (std::size_t i = 0; i < data.occupancy.size(); ++i) {
    const OccupancyCell& cell = data.occupancy[i];
    const std::string cell_path = index_path("/occupancy", i);
    if (cell.kind == OccupancyKind::Ramp && !valid_ramp_direction(cell.yaw)) {
      add_error(issues, cell_path + "/yaw", "occupancy ramp yaw is invalid");
    }
    if (!tile_in_grid(data.height_grid, cell.x, cell.z)) {
      add_error(issues, cell_path, "occupancy cell xz is outside height grid");
    }
  }

  for (std::size_t i = 0; i < data.blockers.size(); ++i) {
    const BlockerDef& blocker = data.blockers[i];
    const std::string blocker_path = index_path("/blockers", i);
    const bool has_base = blocker.base_y.has_value();
    const bool has_top = blocker.top_y.has_value();
    if (has_base != has_top) {
      add_error(issues, blocker_path, "blocker vertical fields require base_y and top_y pair");
    }
    if (blocker.jumpable && !has_base) {
      add_error(issues, blocker_path, "jumpable blocker requires base_y and top_y");
    }
    if (has_base && has_top && *blocker.top_y < *blocker.base_y) {
      add_error(issues, blocker_path, "blocker top_y must be >= base_y");
    }
  }

  std::unordered_map<std::string, std::size_t> event_ids;
  for (std::size_t i = 0; i < data.events.size(); ++i) {
    const EventDef& event = data.events[i];
    const std::string event_path = index_path("/events", i);
    if (event.id.empty()) {
      add_error(issues, event_path + "/id", "event id must not be empty");
    } else if (!event_ids.emplace(event.id, i).second) {
      add_error(issues, event_path + "/id", "event id must be unique");
    }
    for (std::size_t p = 0; p < event.pages.size(); ++p) {
      const EventPage& page = event.pages[p];
      const std::string page_path = index_path(event_path + "/pages", p);
      for (std::size_t c = 0; c < page.conditions.size(); ++c) {
        validate_condition(page.conditions[c], index_path(page_path + "/conditions", c), issues);
      }
      validate_commands(page.commands, page_path + "/commands", issues);
      if (page.graph.has_value()) {
        const EventGraphCompileResult compiled = compile_event_graph(*page.graph);
        for (MapIssue issue : compiled.issues) {
          issue.json_path = page_path + "/graph" + issue.json_path;
          issues.push_back(std::move(issue));
        }
      }
    }
  }

  std::unordered_set<std::string> asset_ids;
  for (std::size_t i = 0; i < data.assets.size(); ++i) {
    const MapAssetRef& asset = data.assets[i];
    const std::string asset_path = index_path("/assets", i);
    if (!asset.id.valid()) {
      add_error(issues, asset_path + "/id", "asset id must not be empty");
    } else if (!asset_ids.insert(asset.id.key()).second) {
      add_error(issues, asset_path + "/id", "asset id must be unique");
    }
  }

  return issues;
}

MapCompileResult compile_map_data(const MapData& data) {
  MapCompileResult preflight;
  preflight.issues = validate_map_structure(data);
  if (map_issues_have_errors(preflight.issues)) return preflight;
  MapData migrated = data;
  apply_v1_height_fallback(migrated);
  for (EventDef& event : migrated.events) {
    for (EventPage& page : event.pages) {
      ensure_page_graph_from_commands(page);
    }
  }
  MapCompileResult result;
  result.issues = validate_map_document(migrated);
  result.ok = !map_issues_have_errors(result.issues);
  if (!result.ok) {
    return result;
  }
  result.runtime.data = std::move(migrated);
  result.runtime.source_revision = 0;
  return result;
}

MapCompileResult compile_map_document(const MapDocument& document) {
  MapCompileResult result = compile_map_data(document.data());
  if (result.ok) {
    result.runtime.source_revision = document.revision();
  }
  return result;
}

MapDocumentLoadResult load_map_document_from_string(std::string_view json_text) {
  MapDocumentLoadResult result;
  try {
    const json root = json::parse(json_text);
    if (!root.contains("schema_version") || !root.at("schema_version").is_number_integer()) {
      add_error(result.issues, "/schema_version", "schema_version is required");
      return result;
    }
    const int version = root.at("schema_version").get<int>();
    if (version != 1 && version != 2 && version != 3 && version != 4 && version != 5) {
      add_error(result.issues, "/schema_version",
                "unsupported schema_version (expected 1, 2, 3, 4, or 5)");
      return result;
    }
  } catch (const std::exception& ex) {
    add_error(result.issues, "/", ex.what());
    return result;
  }

  const MapLoadResult loaded = load_map_from_string(json_text);
  if (!loaded.ok) {
    add_error(result.issues, guess_json_path_from_loader_error(loaded.error),
              loaded.error.empty() ? "map parse failed" : loaded.error);
    return result;
  }

  result.issues = validate_map_document(loaded.map);
  if (map_issues_have_errors(result.issues)) {
    return result;
  }

  result.ok = true;
  result.document = MapDocument(loaded.map);
  return result;
}

MapDocumentLoadResult load_map_document_from_file(const std::string& path) {
  return load_map_document_from_file(path, os_files());
}

MapDocumentLoadResult load_map_document_from_file(const std::string& path, const FileStore& files) {
  const FileReadResult read = files.read(path);
  if (!read.ok) {
    MapDocumentLoadResult result;
    add_error(result.issues, "/",
              read.error.empty() ? "failed to open map file: " + path : read.error);
    return result;
  }
  return load_map_document_from_string(read.bytes.as_text());
}

}  // namespace rat
