---
type: task
area: Engine
status: In progress
task_type: Chore
sprint: Sprint 15
due:
tags: [task]
---

# chore: Edit voxel review polish

Intent: Minor из ревью [[feat-edit-place-3d-terrain-voxels|feat: Edit place 3D terrain voxels]] (Approved): `remove_map_occupancy_cell` на пустой клетке возвращает `ok=false` («not found»); бриф просил no-op. Карта уже не меняется и команда не в undo — выровнять результат на `ok_result()`.

Acceptance: remove missing cell → `ok == true`, occupancy unchanged, no undo entry.

Origin: [[feat-edit-place-3d-terrain-voxels|feat: Edit place 3D terrain voxels]]

## Resolution

## Bugs found

Scheduling: carried from Sprint 14 into [[Sprint 15 — Stabilization]]; original Origin retained.
