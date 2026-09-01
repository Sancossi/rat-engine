#include "rat/map_document.hpp"

#include "rat/map_loader.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <fstream>
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
  const int local_x = tile_x - grid.origin_x;
  const int local_z = tile_z - grid.origin_z;
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
  if (!grid_missing || map.width <= 0 || map.height <= 0) {
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

std::vector<MapIssue> validate_map_document(const MapData& data) {
  std::vector<MapIssue> issues;
  if (data.id.empty()) {
    add_error(issues, "/id", "map id must not be empty");
  }
  if (data.schema_version != 1 && data.schema_version != 2) {
    add_error(issues, "/schema_version", "unsupported schema_version (expected 1 or 2)");
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
    }
  }

  return issues;
}

MapCompileResult compile_map_data(const MapData& data) {
  MapData migrated = data;
  apply_v1_height_fallback(migrated);
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
    if (version != 1 && version != 2) {
      add_error(result.issues, "/schema_version", "unsupported schema_version (expected 1 or 2)");
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
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    MapDocumentLoadResult result;
    add_error(result.issues, "/", "failed to open map file: " + path);
    return result;
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  return load_map_document_from_string(oss.str());
}

}  // namespace rat
