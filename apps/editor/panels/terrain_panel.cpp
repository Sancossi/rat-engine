#include "terrain_panel.hpp"

#include <rat/edit_history.hpp>
#include <rat/height_edit.hpp>

#include <imgui.h>

namespace rat {

void draw_terrain_panel(EditorDocument& document, TerrainPanelState& state) {
  ImGui::Separator();
  ImGui::TextUnformatted("Elevation (Edit)");
  if (ImGui::InputInt("Tile X", &state.tile_x)) {
    // Keep immediate mode state only.
  }
  if (ImGui::InputInt("Tile Z", &state.tile_z)) {
    // Keep immediate mode state only.
  }
  if (state.step <= 0.0f) {
    state.step = 0.25f;
  }
  ImGui::InputFloat("Step", &state.step, 0.05f, 0.25f, "%.3f");
  if (state.step <= 0.0f) {
    state.step = 0.25f;
  }

  const HeightGetResult tile_height =
      get_tile_ground_y(document.visible_data().height_grid, state.tile_x, state.tile_z);
  const TileCoord ramp_tile{state.tile_x, state.tile_z};
  const int ramp_index = find_ramp_index_by_tile(document.visible_data().ramps, ramp_tile);
  if (!state.tile_sync_ready || state.last_tile_x != state.tile_x ||
      state.last_tile_z != state.tile_z) {
    state.tile_sync_ready = true;
    state.last_tile_x = state.tile_x;
    state.last_tile_z = state.tile_z;
    if (tile_height.ok) {
      state.set_y = tile_height.value;
    }
    if (ramp_index >= 0) {
      const RampDef& ramp = document.visible_data().ramps[static_cast<std::size_t>(ramp_index)];
      state.ramp_direction_index = static_cast<int>(ramp.direction);
      state.ramp_low_y = ramp.low_y;
      state.ramp_high_y = ramp.high_y;
    } else if (tile_height.ok) {
      state.ramp_low_y = tile_height.value;
      state.ramp_high_y = tile_height.value;
    }
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

  if (tile_height.ok) {
    ImGui::Text("Ground Y: %.3f", tile_height.value);
    ImGui::TextUnformatted("Tile in range: yes");
  } else {
    ImGui::TextUnformatted("Ground Y: (out of range)");
    ImGui::TextUnformatted("Tile in range: no");
  }

  auto run_height = [&](std::unique_ptr<EditCommand> command) {
    return document.execute(std::move(command)).applied;
  };
  auto sync_set_y = [&]() {
    const HeightGetResult updated =
        get_tile_ground_y(document.visible_data().height_grid, state.tile_x, state.tile_z);
    if (updated.ok) {
      state.set_y = updated.value;
    }
  };

  const bool tile_has_ramp = ramp_index >= 0;
  if (tile_has_ramp) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("- Step")) {
    if (run_height(make_adjust_map_tile_ground_y_command(state.tile_x, state.tile_z, -state.step))) {
      sync_set_y();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("+ Step")) {
    if (run_height(make_adjust_map_tile_ground_y_command(state.tile_x, state.tile_z, state.step))) {
      sync_set_y();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Place cube")) {
    if (run_height(make_place_map_tile_cube_command(state.tile_x, state.tile_z))) {
      sync_set_y();
    }
  }
  if (tile_has_ramp) {
    ImGui::EndDisabled();
  }
  ImGui::InputFloat("Set Y", &state.set_y, 0.05f, 0.25f, "%.3f");
  if (tile_has_ramp) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("Set tile height")) {
    (void)run_height(make_set_map_tile_ground_y_command(state.tile_x, state.tile_z, state.set_y));
  }
  if (tile_has_ramp) {
    ImGui::EndDisabled();
  }

  ImGui::Separator();
  if (ramp_index >= 0) {
    ImGui::TextUnformatted("Ramp on tile: yes");
    ImGui::TextUnformatted("Flat ground editing disabled; use ramp controls.");
  } else {
    ImGui::TextUnformatted("Ramp on tile: no");
  }

  ImGui::Combo("Ramp direction", &state.ramp_direction_index, "North\0East\0South\0West\0");
  ImGui::InputFloat("Ramp low Y", &state.ramp_low_y, 0.05f, 0.25f, "%.3f");
  ImGui::InputFloat("Ramp high Y", &state.ramp_high_y, 0.05f, 0.25f, "%.3f");

  if (ImGui::Button(ramp_index >= 0 ? "Update ramp" : "Add ramp")) {
    RampDef ramp;
    ramp.tile = ramp_tile;
    ramp.direction = static_cast<RampDirection>(state.ramp_direction_index);
    ramp.low_y = state.ramp_low_y;
    ramp.high_y = state.ramp_high_y;
    (void)run_height(make_upsert_map_ramp_command(std::move(ramp)));
  }
  ImGui::SameLine();
  if (ImGui::Button("Remove ramp")) {
    (void)run_height(make_remove_map_ramp_command(ramp_tile));
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Edge fences");
  ImGui::Combo("Edge direction", &state.edge_direction_index, "North\0East\0South\0West\0");
  if (tile_has_ramp) {
    ImGui::BeginDisabled();
  }
  auto upsert_selected_edge = [&](float height) {
    EdgeBarrierDef edge;
    edge.tile = ramp_tile;
    edge.direction = static_cast<RampDirection>(state.edge_direction_index);
    edge.height = height;
    (void)run_height(make_upsert_map_edge_barrier_command(std::move(edge)));
  };
  if (ImGui::Button("Mini 0.45")) {
    upsert_selected_edge(kEdgeBarrierMiniHeight);
  }
  ImGui::SameLine();
  if (ImGui::Button("Full 1.6")) {
    upsert_selected_edge(kEdgeBarrierFullHeight);
  }
  ImGui::SameLine();
  if (ImGui::Button("Remove edge")) {
    (void)run_height(make_remove_map_edge_barrier_command(
        ramp_tile, static_cast<RampDirection>(state.edge_direction_index)));
  }
  if (tile_has_ramp) {
    ImGui::EndDisabled();
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Floor slab");
  if (ImGui::RadioButton("Top Y 1.6", state.slab_top_preset_index == 0)) {
    state.slab_top_preset_index = 0;
    state.slab_top_y = 1.6f;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Top Y 2.0", state.slab_top_preset_index == 1)) {
    state.slab_top_preset_index = 1;
    state.slab_top_y = 2.0f;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Custom", state.slab_top_preset_index == 2)) {
    state.slab_top_preset_index = 2;
  }
  if (state.slab_top_preset_index == 0) {
    state.slab_top_y = 1.6f;
  } else if (state.slab_top_preset_index == 1) {
    state.slab_top_y = 2.0f;
  }
  if (state.slab_top_preset_index == 2) {
    ImGui::InputFloat("Slab top Y", &state.slab_top_y, 0.05f, 0.25f, "%.3f");
  } else {
    ImGui::Text("Slab top Y: %.3f", state.slab_top_y);
  }
  if (state.slab_thickness <= 0.0f) {
    state.slab_thickness = kDefaultFloorSlabThickness;
  }
  ImGui::InputFloat("Slab thickness", &state.slab_thickness, 0.05f, 0.25f, "%.3f");
  if (state.slab_thickness <= 0.0f) {
    state.slab_thickness = kDefaultFloorSlabThickness;
  }
  if (ImGui::Button("Toggle floor slab")) {
    FloorSlabDef slab;
    slab.tile = ramp_tile;
    slab.top_y = state.slab_top_y;
    slab.thickness = state.slab_thickness;
    (void)run_height(make_upsert_map_floor_slab_command(std::move(slab)));
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Ladder");
  ImGui::TextUnformatted("Uses Edge direction (N/E/S/W) from fences.");
  ImGui::InputFloat("Ladder y_lo", &state.ladder_y_lo, 0.05f, 0.25f, "%.3f");
  if (ImGui::InputFloat("Ladder y_hi", &state.ladder_y_hi, 0.05f, 0.25f, "%.3f")) {
    state.ladder_y_hi_user_set = true;
  }
  if (ImGui::Button("Place ladder")) {
    LadderDef ladder;
    ladder.tile = ramp_tile;
    ladder.direction = static_cast<RampDirection>(state.edge_direction_index);
    ladder.y_lo = state.ladder_y_lo;
    ladder.y_hi = state.ladder_y_hi;
    (void)run_height(make_upsert_map_ladder_command(std::move(ladder)));
  }
  ImGui::SameLine();
  if (ImGui::Button("Remove ladder")) {
    (void)run_height(make_remove_map_ladder_command(
        ramp_tile, static_cast<RampDirection>(state.edge_direction_index)));
  }
}

}  // namespace rat
