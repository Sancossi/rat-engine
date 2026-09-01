#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rat {

struct InventoryItem {
  std::string id;
  int quantity = 0;
  bool key_item = false;
};

// Pure game logic state — no GLFW/bgfx. Safe for unit and headless tests.
class GameState {
 public:
  [[nodiscard]] bool get_switch(std::uint32_t id) const;
  void set_switch(std::uint32_t id, bool value);

  [[nodiscard]] int get_variable(std::uint32_t id) const;
  void set_variable(std::uint32_t id, int value);

  void clear();

  void set_map_id(std::string map_id) { map_id_ = std::move(map_id); }
  [[nodiscard]] const std::string& map_id() const { return map_id_; }

  void set_player_position(float x, float y, float z);
  [[nodiscard]] float player_x() const { return player_x_; }
  [[nodiscard]] float player_y() const { return player_y_; }
  [[nodiscard]] float player_z() const { return player_z_; }

  void add_item(const std::string& id, int quantity, bool key_item = false);
  [[nodiscard]] int item_quantity(const std::string& id) const;
  [[nodiscard]] bool has_item(const std::string& id, int min_quantity = 1) const;
  [[nodiscard]] const std::vector<InventoryItem>& inventory() const { return inventory_; }

  // Per-event local switches A–D (RM-like). Key is event id.
  [[nodiscard]] bool get_self_switch(std::string_view event_id, char key) const;
  void set_self_switch(std::string_view event_id, char key, bool value);

  // Stub persistence: in-memory roundtrip (file I/O can wrap the same blob later).
  [[nodiscard]] bool save_to_memory(std::string& out) const;
  [[nodiscard]] bool load_from_memory(std::string_view data);

  [[nodiscard]] std::map<std::uint32_t, bool> debug_switches() const;
  [[nodiscard]] std::map<std::uint32_t, int> debug_variables() const;

 private:
  std::unordered_map<std::uint32_t, bool> switches_;
  std::unordered_map<std::uint32_t, int> variables_;
  std::unordered_map<std::string, std::uint8_t> self_switches_;  // bits 0-3 = A-D
  std::vector<InventoryItem> inventory_;
  std::string map_id_;
  float player_x_ = 0.0f;
  float player_y_ = 0.0f;
  float player_z_ = 0.0f;
};

}  // namespace rat
