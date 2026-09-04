---
type: task
area: Engine
status: In progress
task_type: Research
sprint: Sprint 14
due:
tags: [task]
---

# research: Voxel 3D terrain ADR

Intent: зафиксировать, как хранить и печь **3D-терейн** (несколько walkable объёмов в одной XZ) и **вращаемые рампы**. Playtest: на мост не залезть — 2.5D куб vs slab. Пользователь выбрал воксели, не второй height-grid.

Acceptance: [[ADR-015 Voxel 3D terrain]] с выбором представления (рекомендация: sparse occupancy 1 м = `tile_size`, рампа = клин в клетке с yaw; не animated spin). До ADR в код вокселей не класть. Не Minecraft-продукт, не новый renderer, не field physics. Карты остаются JSON ([[ADR-007 Events and maps stored as JSON]]).

Origin: [[feat: Voxel 3D terrain and rotating ramps]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Related: [[feat: Stacked surfaces caves and basements]], [[Cannot climb onto grey_yard bridge]]. Next: [[feat: Voxel occupancy schema and bake]].

## Resolution

## Bugs found
