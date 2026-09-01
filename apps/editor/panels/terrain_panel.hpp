#pragma once

#include "editor_document.hpp"

namespace rat {

struct TerrainPanelState {
  int tile_x = 0;
  int tile_z = 0;
  float step = 0.25f;
  float set_y = 0.0f;
  int ramp_direction_index = 0;
  int edge_direction_index = 1;
  float ramp_low_y = 0.0f;
  float ramp_high_y = 0.0f;
  bool tile_sync_ready = false;
  int last_tile_x = 0;
  int last_tile_z = 0;
  float slab_top_y = 1.6f;
  float slab_thickness = kDefaultFloorSlabThickness;
  int slab_top_preset_index = 0;
};

void draw_terrain_panel(EditorDocument& document, TerrainPanelState& state);

}  // namespace rat
