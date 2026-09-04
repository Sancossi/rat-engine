---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 14
due:
tags: [task]
---

# feat: Rotating ramp voxels

Intent: рампа — клин в 3D-клетке с **поворотом** (ориентация куска, не крутящийся prop). С неё заходят на соседний воксель другого Y, не только N/E/S/W на одном слое height-grid.

Acceptance: Place ramp voxel; yaw по кликнутой грани (как сейчас N/E/S/W) минимум; если ADR разрешил pitch — отдельный override в панели. Play: цилиндр поднимается по клину на верх соседа. Combo/panel может остаться override. Не Minecraft-sculpt.

Depends: [[feat: Edit place 3D terrain voxels]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Related: [[feat: Place ramp from clicked tile edge]].

## Resolution

## Bugs found
