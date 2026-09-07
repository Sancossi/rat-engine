---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Undo height-grid and edge edits

Intent: Place cube, ramp и edge fence сейчас мутируют карту в обход `EditHistory` (YAGNI [[feat-undo-grow-shrink-and-field-edits|feat: undo grow/shrink and field edits]]). После [[Sprint 5 — Terrain and edge walls]] это основной авторский путь — нужен Ctrl+Z/Y как у blockers/events.

Acceptance: Place cube / `upsert_map_ramp` / upsert+remove `edge_barriers` идут через `EditHistory`; Play не пишет стек; `clear()` на hot-apply/load; headless-тест без окна.

Origin: [[feat-edit-greybox-edge-walls|feat: Edit and greybox edge walls]]

Depends: none. Next: [[feat-mouse-viewport-terrain-edit|feat: Mouse viewport terrain edit]]. Взято в [[Sprint 6 — Viewport map edit]].

## Resolution

Elevation snapshot commands on `EditHistory` (`make_place_map_tile_cube_command`, ramp/edge upsert/remove, step/set ground_y). `EditApplyResult::mutates_elevation` copies `schema_version` / `height_grid` / `ramps` / `edge_barriers` and rebuilds greybox via `apply_edited_map`. Failed `HeightEditResult` does not push undo and restores the before snapshot (legacy schema upgrade must not stick). Play does not write the stack. Review: Approved.

Verify: `.\build\tests\rat_tests.exe "[edit]"`; Ctrl+Z after Place cube / Mini fence in Edit.

## Bugs found

none.
