# Task 25: feat: Mouse viewport map edit — Report

## What I implemented
- Added new `rat_core` module `viewport_edit` with:
  - screen-pixel to world ray unproject via `OrthoCamera.view/proj` and ground-plane hit (`y=0` default),
  - map object picking on overlap (blocker AABB / event tile or volume) with nearest-center resolve and blocker-first tie,
  - world→tile conversion (`floor(xz / tile_size)`),
  - tile delta helper for drag,
  - click-intent resolver for `Select`, `PlaceBlocker`, `PlaceEvent`.
- Wired `apps/editor` mouse edit flow (Edit mode only, and only when `!ImGui::GetIO().WantCaptureMouse`):
  - left press: select/deselect/place based on core click intent,
  - left drag: tile-by-tile move commands for selected blocker/event (not per pixel),
  - left release: drag stop.
- Added editor tool switch UI (`Select`, `Place blocker`, `Place event`) with `Select` default.
- Reused existing `EditHistory` commands (`make_place_*`, `make_move_*`) and apply path (`edit_history_.execute` + `apply_edited_map`).
- Ensured click on existing object selects even in place tools, and selection sync updates `selected_blocker_`/`selected_event_`.
- No Play-mode map edit mouse behavior added.

## What I tested and results
- Focused:
  - `build/tests/rat_tests.exe [viewport_edit]`
  - Result: pass (`12 assertions`, `4 test cases`).
- Full:
  - `build/tests/rat_tests.exe`
  - Result: pass (`2124 assertions`, `268 test cases`).

## TDD Evidence (RED then GREEN)
- RED:
  - Added `tests/viewport_edit_test.cpp` + CMake registration first.
  - Ran focused target before implementation; failed on missing `rat/viewport_edit.hpp` (expected missing-feature failure).
- GREEN:
  - Implemented `viewport_edit` core API and editor wiring.
  - Re-ran focused tests until green.
  - Re-ran full `rat_tests` and got all green.

## Files changed
- `src/engine/include/rat/viewport_edit.hpp` (new)
- `src/engine/src/viewport_edit.cpp` (new)
- `src/engine/CMakeLists.txt`
- `tests/viewport_edit_test.cpp` (new)
- `tests/CMakeLists.txt`
- `apps/editor/editor_app.hpp`
- `apps/editor/editor_app.cpp`

## Self-review findings
- Completeness: all brief items implemented for this card scope (mouse select/place/move in Edit only, core unproject/pick/tile helpers, focused+full tests).
- Quality: core math/pick logic stays in `rat_core`; GLFW/ImGui handling stays in `apps/editor`.
- YAGNI: no new undo command types, no terrain mouse/height/edge/cube scope creep, no Play input changes.
- Behavior checks: place tools on overlap select existing object; drag emits tile-step move commands.

## Concerns
- `unproject_to_ground_plane` includes a documented Z-sign normalization step (`-world_z`) to match map tile-space convention observed in this codebase/tests. No failing tests remain, but this convention should stay consistent if camera matrix conventions change later.

## Task 25 follow-up: unproject Z fix
- Added TDD coverage in `tests/viewport_edit_test.cpp`:
  - off-center top-down pixel expects world XZ from ortho extents,
  - Tilt45 center pixel hits `params.focus` XZ,
  - ThreeQuarter center pixel hits `params.focus` XZ (looser margin).
- Reworked `unproject_to_ground_plane` in `src/engine/src/viewport_edit.cpp` to derive ray origin/direction from `camera.view` basis + `camera.proj` extents; removed unconditional `-world_z`.
- Added in-code derivation comment for recovering `eye` from `look_at` translation terms.
- Verification:
  - `build/tests/rat_tests.exe "[viewport_edit]"` → pass (`22 assertions`, `7 test cases`).
  - `build/tests/rat_tests.exe` → pass (`2183 assertions`, `275 test cases`).
