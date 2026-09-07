---
type: task
area: Engine
status: In progress
task_type: Chore
sprint: Sprint 15
due:
tags: [task]
---

# chore: Ramp voxel review polish

Intent: Minor из ревью [[feat-rotating-ramp-voxels|feat: Rotating ramp voxels]] (Approved).

- Place по боковой грани: yaw с `nearest_tile_edge` занятой клетки смотрит от солида; брать yaw от нормали грани (high к солиду) или от target-клетки.
- Greybox: вырожденные side quads у occupancy-ramp (`terrain_geometry.cpp` East south / West north).
- Unit: omit side fence только на high-face солида, остальные три на месте.

Acceptance: side-face place yaw ведёт на соседний solid; wedge greybox без дублей вершин; `[collision]` ловит лишний omit.

Origin: [[feat-rotating-ramp-voxels|feat: Rotating ramp voxels]]

## Resolution

## Bugs found

Scheduling: carried from Sprint 14 into [[Sprint 15 — Stabilization]]; original Origin retained.


Follow-up reproduced during stabilization: [[slab-ramp-height-mismatch-removes-fence]].
