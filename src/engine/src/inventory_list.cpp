#include "rat/inventory_list.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace rat {

std::vector<std::string> format_inventory_rows(const std::vector<InventoryItem>& items) {
  if (items.empty()) {
    return {"(empty)"};
  }
  std::vector<std::string> rows;
  rows.reserve(items.size());
  for (const InventoryItem& item : items) {
    std::ostringstream line;
    line << item.id << "  x" << item.quantity;
    if (item.key_item) {
      line << "  key";
    }
    rows.push_back(line.str());
  }
  return rows;
}

}  // namespace rat
