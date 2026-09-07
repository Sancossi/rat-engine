---
type: task
area: Engine
status: Done
task_type: Research
sprint: Sprint 14
due:
tags: [task]
---

# research: Voxel 3D terrain ADR

Intent: зафиксировать, как хранить и печь **3D-терейн** (несколько walkable объёмов в одной XZ) и **вращаемые рампы**. Playtest: на мост не залезть — 2.5D куб vs slab. Пользователь выбрал воксели, не второй height-grid.

Acceptance: [[ADR-015 Voxel 3D terrain]] с выбором представления (рекомендация: sparse occupancy 1 м = `tile_size`, рампа = клин в клетке с yaw; не animated spin). До ADR в код вокселей не класть. Не Minecraft-продукт, не новый renderer, не field physics. Карты остаются JSON ([[ADR-007 Events and maps stored as JSON]]).

Origin: [[feat-voxel-3d-terrain-and-rotating-ramps|feat: Voxel 3D terrain and rotating ramps]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Related: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]], [[cannot-climb-onto-grey-yard-bridge|Cannot climb onto grey_yard bridge]]. Next: [[feat-voxel-occupancy-schema-and-bake|feat: Voxel occupancy schema and bake]].

## Resolution

[[ADR-015 Voxel 3D terrain]] Accepted: sparse occupancy 1 м (`solid` куб / `ramp` yaw-клин), dual-read schema 1–4, bake union в существующий `CollisionWorld`. Pitch нет в v1. `floor_slabs` остаются (0.25 м ≠ куб 1 м). Кода вокселей в этом срезе нет. Review: Approved.

Verify: открыть ADR-015; строка Voxel occupancy в [[Systems Index]].

## Bugs found

none. Follow-up overlap/precedence зафиксирован в ADR (occupancy wins на том же 1 м AABB) для [[feat-voxel-occupancy-schema-and-bake|feat: Voxel occupancy schema and bake]].
