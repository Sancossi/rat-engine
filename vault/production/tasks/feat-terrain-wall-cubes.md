---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 5
due:
tags: [task]
---

# feat: Terrain wall cubes

Intent: клетка height-grid как standable куб-стена. Сбоку не пройти, если выше `max_step_up`; сверху можно стоять. Сейчас greybox рисует только верхние квады — куб сбоку не виден.

Acceptance: боковые грани на разрывах `ground_y`; инструмент «поставить куб» поднимает клетку на 1.0 (не на ramp); headless-тест геометрии + height edit.

Depends: none. Next: [[feat: Grid edge barriers]]. Взято в [[Sprint 5 — Terrain and edge walls]].

## Resolution

`build_terrain_side_faces` emits vertical quads on east/south shared edges where corner Ys differ (trapezoid OK for ramps). Greybox `set_terrain_map` draws those faces with the same fill as tops and fail-closes if tops+sides overflow uint16. **Place cube** raises a flat tile by exactly 1.0 (`place_map_tile_cube` / `EventRuntime::place_tile_cube`); ramp tiles reject without mutation. Collision is existing too-high step-up — no physics change.

Verify: `.\build\tests\rat_tests.exe "[terrain]"` and `"[height_edit]"`; Edit → Elevation → Place cube.

## Bugs found

none.

