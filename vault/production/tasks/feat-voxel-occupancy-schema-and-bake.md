---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 14
due:
tags: [task]
---

# feat: Voxel occupancy schema and bake

Intent: после ADR — JSON occupancy (воксель на `(x,y,z)` или эквивалент) + parse/dump + bake в collision solids. Height-grid может остаться compatibility или уйти — как решит ADR.

Acceptance: schema bump; roundtrip; два вокселя друг над другом в одной XZ — тонкий/куб не fill-to-Y=0 между ними; unit bake. Не Edit-инструмент (следующая карточка). Не field physics.

Depends: [[research: Voxel 3D terrain ADR]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Related: [[feat: Voxel 3D terrain and rotating ramps]].

## Resolution

## Bugs found
