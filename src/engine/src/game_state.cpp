#include "rat/game_state.hpp"

namespace rat {

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
  inventory_.clear();
  map_id_.clear();
  player_x_ = player_y_ = player_z_ = 0.0f;
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

}  // namespace rat
