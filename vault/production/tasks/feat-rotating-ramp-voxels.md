---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 14
due:
tags: [task]
---

# feat: Rotating ramp voxels

Intent: рампа — клин в 3D-клетке с **поворотом** (ориентация куска, не крутящийся prop). С неё заходят на соседний воксель другого Y, не только N/E/S/W на одном слое height-grid.

Acceptance: Place ramp voxel; yaw по кликнутой грани (как сейчас N/E/S/W) минимум; если ADR разрешил pitch — отдельный override в панели. Play: цилиндр поднимается по клину на верх соседа. Combo/panel может остаться override. Не Minecraft-sculpt.

Depends: [[feat: Edit place 3D terrain voxels]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Related: [[feat: Place ramp from clicked tile edge]]. Follow-up: [[chore: Ramp voxel review polish]]. Follow-up: [[Cannot climb onto grey_yard bridge]] — omit `floor_slab` side fence на high-side occupancy-ramp (как у occupancy solid), иначе crest на плиту моста упрётся в забор.

## Resolution

Terrain **Place ramp voxel** пишет `occupancy` `kind: ramp`, yaw с `nearest_tile_edge`. Pitch нет. Legacy Place ramp → `ramps[]`. Greybox-клин, undo, цилиндр +1 м на верх соседа-solid. Side fence солида у high-side рампы снимается — иначе crest не выходит. Review: Approved.

Verify: Edit → Terrain → Place ramp voxel к кубу; Play заход на верх. `.\build\tests\rat_tests.exe "[map],[collision],[viewport],[edit],[terrain]"`.

## Bugs found

none.
