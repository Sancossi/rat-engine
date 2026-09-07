---
type: task
area: Engine
status: Done
review: Approved
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

Implemented in 2a44876; independent review Approved. Full verification 698/698 C++ tests passed, focused geometry/bridge run 6 tests / 1,314 assertions. See [[s15-geometry]] for shared evidence.

## Bugs found

Slab-height mismatch and review arithmetic findings resolved; see [[s15-geometry]].

Scheduling: carried from Sprint 14 into [[Sprint 15 — Stabilization]]; original Origin retained.


Follow-up reproduced during stabilization: [[slab-ramp-height-mismatch-removes-fence]].
