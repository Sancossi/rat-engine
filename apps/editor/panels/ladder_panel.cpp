#include "ladder_panel.hpp"

#include <rat/edit_history.hpp>
#include <rat/height_edit.hpp>

#include <imgui.h>

namespace rat {

void draw_ladder_panel(EditorDocument& document, TerrainPanelState& state) {
  ImGui::Separator();
  ImGui::TextUnformatted("Ladder");
  if (ImGui::InputInt("Tile X", &state.tile_x)) {
    // Keep immediate mode state only.
  }
  if (ImGui::InputInt("Tile Z", &state.tile_z)) {
    // Keep immediate mode state only.
  }
  ImGui::Combo("Edge direction", &state.edge_direction_index, "North\0East\0South\0West\0");

  if (!state.ladder_tile_sync_ready || state.ladder_last_tile_x != state.tile_x ||
      state.ladder_last_tile_z != state.tile_z) {
    state.ladder_tile_sync_ready = true;
    state.ladder_last_tile_x = state.tile_x;
    state.ladder_last_tile_z = state.tile_z;
    if (!state.ladder_y_hi_user_set) {
      float suggested_hi = state.ladder_y_hi;
      bool found_slab = false;
      for (const FloorSlabDef& slab : document.visible_data().floor_slabs) {
        if (slab.tile.x != state.tile_x || slab.tile.z != state.tile_z) {
          continue;
        }
        if (slab.top_y <= state.ladder_y_lo) {
          continue;
        }
        if (!found_slab || slab.top_y > suggested_hi) {
          suggested_hi = slab.top_y;
          found_slab = true;
        }
      }
      if (found_slab) {
        state.ladder_y_hi = suggested_hi;
      }
    }
  }

  ImGui::InputFloat("Ladder y_lo", &state.ladder_y_lo, 0.05f, 0.25f, "%.3f");
  if (ImGui::InputFloat("Ladder y_hi", &state.ladder_y_hi, 0.05f, 0.25f, "%.3f")) {
    state.ladder_y_hi_user_set = true;
  }
  const TileCoord ladder_tile{state.tile_x, state.tile_z};
  if (ImGui::Button("Place ladder")) {
    LadderDef ladder;
    ladder.tile = ladder_tile;
    ladder.direction = static_cast<RampDirection>(state.edge_direction_index);
    ladder.y_lo = state.ladder_y_lo;
    ladder.y_hi = state.ladder_y_hi;
    (void)document.execute(make_upsert_map_ladder_command(std::move(ladder)));
  }
  ImGui::SameLine();
  if (ImGui::Button("Remove ladder")) {
    (void)document.execute(make_remove_map_ladder_command(
        ladder_tile, static_cast<RampDirection>(state.edge_direction_index)));
  }
}

}  // namespace rat
