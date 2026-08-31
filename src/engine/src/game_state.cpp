#include "rat/game_state.hpp"

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
    return consumed == token.size();
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
  std::ostringstream oss;
  oss << "RATSAVE1\n";
  oss << "map " << map_id_ << '\n';
  oss << "pos " << player_x_ << ' ' << player_y_ << ' ' << player_z_ << '\n';
  for (const auto& [id, value] : switches_) {
    oss << "sw " << id << ' ' << (value ? 1 : 0) << '\n';
  }
  for (const auto& [id, value] : variables_) {
    oss << "var " << id << ' ' << value << '\n';
  }
  for (const auto& [event_id, bits] : self_switches_) {
    oss << "ss " << event_id << ' ' << static_cast<unsigned>(bits) << '\n';
  }
  for (const auto& item : inventory_) {
    oss << "item " << item.id << ' ' << item.quantity << ' ' << (item.key_item ? 1 : 0) << '\n';
  }
  out = oss.str();
  return true;
}

bool GameState::load_from_memory(std::string_view data) {
  clear();
  if (data.empty()) {
    return false;
  }

  std::istringstream iss{std::string(data)};
  std::string line;
  if (!std::getline(iss, line) || line != "RATSAVE1") {
    return false;
  }

  while (std::getline(iss, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream ls(line);
    std::string tag;
    ls >> tag;
    if (tag == "map") {
      std::getline(ls >> std::ws, map_id_);
    } else if (tag == "pos") {
      std::string sx;
      std::string sy;
      std::string sz;
      if (!(ls >> sx >> sy >> sz)) {
        return false;
      }
      if (!parse_float(sx, player_x_) || !parse_float(sy, player_y_) || !parse_float(sz, player_z_)) {
        return false;
      }
    } else if (tag == "sw") {
      std::uint32_t id = 0;
      int value = 0;
      std::string sid;
      std::string sval;
      if (!(ls >> sid >> sval) || !parse_u32(sid, id) || !parse_int(sval, value)) {
        return false;
      }
      switches_[id] = value != 0;
    } else if (tag == "var") {
      std::uint32_t id = 0;
      int value = 0;
      std::string sid;
      std::string sval;
      if (!(ls >> sid >> sval) || !parse_u32(sid, id) || !parse_int(sval, value)) {
        return false;
      }
      variables_[id] = value;
    } else if (tag == "ss") {
      std::string event_id;
      unsigned bits = 0;
      if (!(ls >> event_id >> bits) || event_id.empty()) {
        return false;
      }
      self_switches_[std::move(event_id)] = static_cast<std::uint8_t>(bits & 0x0Fu);
    } else if (tag == "item") {
      std::string id;
      int quantity = 0;
      int key = 0;
      std::string sq;
      std::string sk;
      if (!(ls >> id >> sq >> sk) || !parse_int(sq, quantity) || !parse_int(sk, key)) {
        return false;
      }
      if (!id.empty() && quantity > 0) {
        inventory_.push_back(InventoryItem{std::move(id), quantity, key != 0});
      }
    } else {
      return false;
    }
  }
  return true;
}

std::map<std::uint32_t, bool> GameState::debug_switches() const {
  return std::map<std::uint32_t, bool>(switches_.begin(), switches_.end());
}

std::map<std::uint32_t, int> GameState::debug_variables() const {
  return std::map<std::uint32_t, int>(variables_.begin(), variables_.end());
}

}  // namespace rat
