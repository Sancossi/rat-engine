#pragma once

#include "editor_document.hpp"

#include <optional>
#include <string>

namespace rat {

struct BlockerPanelState {
  std::optional<BlockerDef> field_origin;
  int field_origin_index = -1;
};

void draw_blocker_panel(EditorDocument& document, BlockerPanelState& state, float sampled_ground_y,
                        std::string& last_error);

}  // namespace rat
