# Task 28: feat: Mouse viewport terrain edit — Report

## What I implemented
- Extended `ViewportTool` / `ViewportClickActionKind` with `PlaceCube` and `PlaceFence`.
- `resolve_viewport_click` still selects on blocker/event hits (including PlaceCube / PlaceFence). Empty ground returns the clicked tile via existing `world_to_tile_xz` (no unproject changes).
- Ramp tiles: resolve still returns `PlaceCube`; `place_map_tile_cube` rejects without raising (same as ImGui).
- Editor radios: **Place cube** and **Fence** next to Select / Place blocker / Place event. Fence preset Mini 0.45 / Full 1.6 / Remove (default Mini). Edge direction stays the existing combo.
- Viewport PlaceCube / PlaceFence clicks go through `run_height_history` + existing `make_place_map_tile_cube_command` / `make_upsert_map_edge_barrier_command` / `make_remove_map_edge_barrier_command`. Clicked tile syncs `height_tile_x_` / `height_tile_z_` / `height_set_y_`. One command per click (no drag-paint).
- `WantCaptureMouse` / Play unchanged. Greybox still rebuilds via `apply_edited_map` + `mutates_elevation`.

## What I tested and results
- Focused: `build/tests/rat_tests.exe "[viewport_edit]"` → pass (`47 assertions`, `12 test cases`).
- Full: `build/tests/rat_tests.exe` → pass (`2209 assertions`, `283 test cases`).
- `rat-editor` compiled after editor wiring.

## TDD Evidence (RED then GREEN)
- RED:
  - Added PlaceCube / PlaceFence tests first. Compile failed on missing enumerators (`C2065` / `C2838`).
  - After adding enumerators only (no switch cases): tests compiled; empty-ground clicks failed `REQUIRE(action.kind == PlaceCube/PlaceFence)` with `0 == 6` / `0 == 7` (`None`).
- GREEN:
  - Added switch cases in `resolve_viewport_click`. Focused `[viewport_edit]` green.
  - Wired editor commands/UI. Full `rat_tests` green.

## Files changed
- `src/engine/include/rat/viewport_edit.hpp`
- `src/engine/src/viewport_edit.cpp`
- `tests/viewport_edit_test.cpp`
- `apps/editor/editor_app.hpp`
- `apps/editor/editor_app.cpp`
- `.superpowers/sdd/task-28-mouse-viewport-terrain-edit-report.md`

## Self-review findings
- Completeness: click-to-place cube and fence preset on selected edge direction; object hits still select; ramp cube/fence reject via existing commands.
- Quality: pick stays in `rat_core`; GLFW/ImGui and presets stay in `apps/editor`.
- YAGNI: no new undo command types, no click-to-pick edge, no camera/unproject invert, no schema v3 / jump / Play mouse.
- Vault card left `In progress`.

## Concerns
- None. Fence direction/preset are editor state, not part of the pick, matching the brief.
