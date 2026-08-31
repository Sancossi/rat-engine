# Task 26: feat: Undo height-grid and edge edits — Report

## What I implemented
- Extended `EditApplyResult` with `mutates_elevation` and propagated this flag from commands.
- Added elevation-aware `EditCommand` factories in `rat_core`:
  - `make_set_map_tile_ground_y_command`
  - `make_adjust_map_tile_ground_y_command`
  - `make_place_map_tile_cube_command`
  - `make_upsert_map_ramp_command` / `make_remove_map_ramp_command`
  - `make_upsert_map_edge_barrier_command` / `make_remove_map_edge_barrier_command`
- Implemented an elevation snapshot command (`ReplaceElevationSnapshotCommand`) that:
  - applies height/ramp/edge edits through existing `height_edit` functions,
  - stores before/after elevation snapshots for deterministic undo/redo,
  - does not push to undo stack on failed `HeightEditResult`,
  - keeps `mutates_blockers=false` and `mutates_events=false`.
- Updated `EditHistory::execute`/`redo` to skip history push when a command apply fails.
- Extended editor apply path:
  - `EditorApp::apply_edited_map` now handles `mutates_elevation` by applying `schema_version`, `height_grid`, `ramps`, `edge_barriers`, then rebuilding terrain/surface sync.
- Routed height/ramp/edge ImGui actions through `EditHistory`:
  - `- Step`, `+ Step`, `Set tile height`, `Place cube`,
  - `Add/Update ramp`, `Remove ramp`,
  - `Mini 0.45`, `Full 1.6`, `Remove edge`.

## What I tested and results
- Build:
  - `cmake --build build --target rat_tests` (via VS dev environment) — pass.
- Focused:
  - `.\build\tests\rat_tests.exe "[edit]"` — pass (`187 assertions`, `16 test cases`).
- Full:
  - `.\build\tests\rat_tests.exe` — pass (`2160 assertions`, `271 test cases`).

## TDD evidence (RED then GREEN)
- RED:
  - Added elevation undo tests first in `tests/edit_history_test.cpp`.
  - Ran build; compile failed on missing elevation command factories and `mutates_elevation` (expected missing-feature failure).
- GREEN:
  - Implemented elevation commands + editor wiring.
  - Rebuilt and reran focused `[edit]` and full `rat_tests` to green.

## Files changed
- `src/engine/include/rat/edit_history.hpp`
- `src/engine/src/edit_history.cpp`
- `src/engine/include/rat/event_runtime.hpp`
- `src/engine/src/event_runtime.cpp`
- `apps/editor/editor_app.cpp`
- `tests/edit_history_test.cpp`

## Self-review findings
- Scope: task requirements covered for undo/redo on height-grid, ramps, and edge barriers through `EditHistory` in Edit mode.
- Correctness: failed elevation edits return `applied=false` and do not create undo entries.
- Integration: `apply_edited_map` now applies elevation mutations explicitly, so undo/redo updates runtime terrain state.
- Non-goals preserved: no mouse viewport terrain editing changes, no schema v3, no jump physics edits.

## Concerns
- None at this scope; behavior is covered by focused edit-history tests and full suite pass.

## Task 26 fix after review (failed elevation rollback)
- Added RED test `failed place cube on legacy map keeps elevation snapshot unchanged` in `tests/edit_history_test.cpp`.
- RED result before fix: `[edit]` failed on `REQUIRE( map.schema_version == 1 )` with actual `2 == 1`.
- Fixed `ReplaceElevationSnapshotCommand::apply` in `src/engine/src/edit_history.cpp`: when `edit_(map)` returns `!ok`, it now restores `schema_version`, `height_grid`, `ramps`, and `edge_barriers` from the captured before-snapshot before returning failure.
- GREEN result after fix:
  - `.\build\tests\rat_tests.exe "[edit]"` — pass (`200 assertions`, `17 test cases`).
  - `.\build\tests\rat_tests.exe` — pass (`2173 assertions`, `272 test cases`).
