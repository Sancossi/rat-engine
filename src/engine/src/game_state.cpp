#include "rat/game_state.hpp"
#include "strict_json.hpp"

#include <charconv>
#include <map>
#include <sstream>

namespace rat {

namespace {

bool parse_u32(std::string_view token, std::uint32_t& out) {
  const auto* begin = token.data();
  const auto* end = begin + token.size();
  const auto [ptr, ec] = std::from_chars(begin, end, out);
  return ec == std::errc{} && ptr == end;
}

bool parse_int(std::string_view token, int& out) {
  const auto* begin = token.data();
  const auto* end = begin + token.size();
  const auto [ptr, ec] = std::from_chars(begin, end, out);
  return ec == std::errc{} && ptr == end;
}

bool parse_float(std::string_view token, float& out) {
  try {
    std::size_t consumed = 0;
    out = std::stof(std::string(token), &consumed);
    return consumed == token.size() && std::isfinite(out);
  } catch (...) {
    return false;
  }
}

}  // namespace

bool GameState::get_switch(std::uint32_t id) const {
  const auto it = switches_.find(id);
  return it != switches_.end() && it->second;
}

void GameState::set_switch(std::uint32_t id, bool value) {
  switches_[id] = value;
}

int GameState::get_variable(std::uint32_t id) const {
  const auto it = variables_.find(id);
  return it == variables_.end() ? 0 : it->second;
}

void GameState::set_variable(std::uint32_t id, int value) {
  variables_[id] = value;
}

void GameState::clear() {
  switches_.clear();
  variables_.clear();
  self_switches_.clear();
  inventory_.clear();
  map_id_.clear();
  player_x_ = player_y_ = player_z_ = 0.0f;
}

bool GameState::get_self_switch(std::string_view event_id, char key) const {
  const int bit = key - 'A';
  if (bit < 0 || bit > 3 || event_id.empty()) {
    return false;
  }
  const auto it = self_switches_.find(std::string(event_id));
  if (it == self_switches_.end()) {
    return false;
  }
  return (it->second & (1u << bit)) != 0;
}

void GameState::set_self_switch(std::string_view event_id, char key, bool value) {
  const int bit = key - 'A';
  if (bit < 0 || bit > 3 || event_id.empty()) {
    return;
  }
  std::uint8_t& bits = self_switches_[std::string(event_id)];
  if (value) {
    bits = static_cast<std::uint8_t>(bits | (1u << bit));
  } else {
    bits = static_cast<std::uint8_t>(bits & ~(1u << bit));
  }
}

void GameState::set_player_position(float x, float y, float z) {
  player_x_ = x;
  player_y_ = y;
  player_z_ = z;
}

void GameState::add_item(const std::string& id, int quantity, bool key_item) {
  if (quantity == 0 || id.empty()) {
    return;
  }
  for (auto& item : inventory_) {
    if (item.id == id) {
      item.quantity += quantity;
      if (item.quantity < 0) {
        item.quantity = 0;
      }
      item.key_item = item.key_item || key_item;
      return;
    }
  }
  if (quantity > 0) {
    inventory_.push_back(InventoryItem{id, quantity, key_item});
  }
}

int GameState::item_quantity(const std::string& id) const {
  for (const auto& item : inventory_) {
    if (item.id == id) {
      return item.quantity;
    }
  }
  return 0;
}

bool GameState::has_item(const std::string& id, int min_quantity) const {
  return item_quantity(id) >= min_quantity;
}

bool GameState::save_to_memory(std::string& out) const {
  using json = nlohmann::json;
  if (map_id_.empty() || !std::isfinite(player_x_) || !std::isfinite(player_y_) ||
      !std::isfinite(player_z_)) return false;
  try {
    json root{{"schema_version", 2}, {"map_id", map_id_},
              {"position", {{"x", player_x_}, {"y", player_y_}, {"z", player_z_}}},
              {"switches", json::array()}, {"variables", json::array()},
              {"self_switches", json::array()}, {"inventory", json::array()}};
    for (const auto& [id, value] : debug_switches())
      root["switches"].push_back({{"id", id}, {"value", value}});
    for (const auto& [id, value] : debug_variables())
      root["variables"].push_back({{"id", id}, {"value", value}});
    const std::map<std::string, std::uint8_t> ordered(self_switches_.begin(), self_switches_.end());
    for (const auto& [id, bits] : ordered)
      root["self_switches"].push_back({{"event_id", id}, {"bits", bits}});
    for (const auto& item : inventory_)
      root["inventory"].push_back({{"id", item.id}, {"quantity", item.quantity}, {"key_item", item.key_item}});
    out = root.dump(2);
    return true;
  } catch (const std::exception&) { return false; }
}

bool GameState::load_from_memory(std::string_view data) {
  using namespace detail;
  GameState parsed;
  std::unordered_set<std::string> item_ids;
  try {
    if (!data.starts_with("RATSAVE1")) {
      const auto root = parse_unique_json(data);
      exact_fields(root, {"schema_version", "map_id", "position", "switches", "variables", "self_switches", "inventory"});
      if (checked_number<int>(root.at("schema_version")) != 2) return false;
      parsed.map_id_ = root.at("map_id").get<std::string>();
      const auto& pos = root.at("position");
      exact_fields(pos, {"x", "y", "z"});
      parsed.player_x_ = checked_number<float>(pos.at("x"));
      parsed.player_y_ = checked_number<float>(pos.at("y"));
      parsed.player_z_ = checked_number<float>(pos.at("z"));
      for (const char* name : {"switches", "variables", "self_switches", "inventory"})
        if (!root.at(name).is_array()) return false;
      for (const auto& entry : root.at("switches")) {
        exact_fields(entry, {"id", "value"});
        if (!parsed.switches_.emplace(checked_number<std::uint32_t>(entry.at("id")), entry.at("value").get<bool>()).second) return false;
      }
      for (const auto& entry : root.at("variables")) {
        exact_fields(entry, {"id", "value"});
        if (!parsed.variables_.emplace(checked_number<std::uint32_t>(entry.at("id")), checked_number<int>(entry.at("value"))).second) return false;
      }
      for (const auto& entry : root.at("self_switches")) {
        exact_fields(entry, {"event_id", "bits"});
        const auto id = entry.at("event_id").get<std::string>();
        const auto bits = checked_number<std::uint8_t>(entry.at("bits"));
        if (id.empty() || bits > 15 || !parsed.self_switches_.emplace(id, bits).second) return false;
      }
      for (const auto& entry : root.at("inventory")) {
        exact_fields(entry, {"id", "quantity", "key_item"});
        InventoryItem item{entry.at("id").get<std::string>(), checked_number<int>(entry.at("quantity")), entry.at("key_item").get<bool>()};
        if (item.id.empty() || item.quantity < 0 || !item_ids.insert(item.id).second) return false;
        parsed.inventory_.push_back(std::move(item));
      }
    } else {
      std::istringstream input{std::string(data)};
      std::string line;
      if (!std::getline(input, line)) return false;
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line != "RATSAVE1") return false;
      bool seen_map = false, seen_position = false;
      while (std::getline(input, line)) {
        if (line.empty() || line == "\r") continue;
        std::istringstream fields(line);
        std::string tag, a, b, c, extra;
        fields >> tag;
        if (tag == "map") {
          if (seen_map) return false;
          seen_map = true;
          std::getline(fields >> std::ws, parsed.map_id_);
          if (!parsed.map_id_.empty() && parsed.map_id_.back() == '\r') parsed.map_id_.pop_back();
          continue;
        }
        if (!(fields >> a >> b)) return false;
        if (tag == "pos") {
          if (seen_position || !(fields >> c) || !parse_float(a, parsed.player_x_) ||
              !parse_float(b, parsed.player_y_) || !parse_float(c, parsed.player_z_)) return false;
          seen_position = true;
        } else if (tag == "sw" || tag == "var") {
          std::uint32_t id;
          int value;
          if (!parse_u32(a, id) || !parse_int(b, value)) return false;
          if (tag == "sw") {
            if ((value != 0 && value != 1) || !parsed.switches_.emplace(id, value != 0).second) return false;
          } else if (!parsed.variables_.emplace(id, value).second) return false;
        } else if (tag == "ss") {
          std::uint32_t bits;
          if (!parse_u32(b, bits) || bits > 15 || !parsed.self_switches_.emplace(a, static_cast<std::uint8_t>(bits)).second) return false;
        } else if (tag == "item") {
          int quantity, key;
          if (!(fields >> c) || !parse_int(b, quantity) || !parse_int(c, key) || quantity < 0 ||
              (key != 0 && key != 1) || !item_ids.insert(a).second) return false;
          parsed.inventory_.push_back({a, quantity, key != 0});
        } else return false;
        if (fields >> extra) return false;
      }
      if (!seen_map || !seen_position) return false;
    }
    if (parsed.map_id_.empty()) return false;
    *this = std::move(parsed);
    return true;
  } catch (const std::exception&) { return false; }
}

std::map<std::uint32_t, bool> GameState::debug_switches() const {
  return std::map<std::uint32_t, bool>(switches_.begin(), switches_.end());
}

std::map<std::uint32_t, int> GameState::debug_variables() const {
  return std::map<std::uint32_t, int>(variables_.begin(), variables_.end());
}

}  // namespace rat
