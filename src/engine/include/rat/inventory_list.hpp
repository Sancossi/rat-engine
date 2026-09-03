#pragma once

#include "rat/game_state.hpp"

#include <string>
#include <vector>

namespace rat {

[[nodiscard]] std::vector<std::string> format_inventory_rows(
    const std::vector<InventoryItem>& items);

}  // namespace rat
