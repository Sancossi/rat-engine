---
type: task
area: Engine
status: In review
review: In review
task_type: Chore
sprint: Sprint 15
due:
tags: [task]
---

# chore: Occupancy review test polish

Intent: Minor из ревью [[feat-voxel-occupancy-schema-and-bake|feat: Voxel occupancy schema and bake]] (Approved): вернуть `json_text` identity после save/load в mapdoc-тесте (или задокументировать, почему только второй цикл) и добавить unit, что occupancy xz вне `height_grid` даёт `/occupancy/0`.

Acceptance: `[mapdoc]` ловит occupancy cell outside grid; schema-2 identity не ослаблен без причины в комментарии или снова `again.json_text == serialized.json_text`.

Origin: [[feat-voxel-occupancy-schema-and-bake|feat: Voxel occupancy schema and bake]]

## Resolution

## Bugs found

Scheduling: carried from Sprint 14 into [[Sprint 15 — Stabilization]]; original Origin retained.
