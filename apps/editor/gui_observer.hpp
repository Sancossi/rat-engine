#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rat {

struct GuiItemObservation {
  std::uint32_t id = 0;
  std::string window;
  std::string label;
  float min_x = 0, min_y = 0, max_x = 0, max_y = 0;
  float clip_min_x = 0, clip_min_y = 0, clip_max_x = 0, clip_max_y = 0;
  bool visible = false;
  bool enabled = false;
};

// Instrumentation reads ImGui's actual submitted items. It never activates them.
void begin_gui_observation();
const std::vector<GuiItemObservation>& gui_items();

}  // namespace rat
