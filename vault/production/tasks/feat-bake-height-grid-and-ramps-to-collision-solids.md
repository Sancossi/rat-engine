---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Bake height-grid and ramps to collision solids

Intent: после [[edge-walls-passable-from-adjacent-side]] подключить кубы height-grid и рампы к тому же `CollisionWorld`, что и заборы. Edit-представление не меняется; bake на load/hot-apply. Origin: collider design (chat).

Acceptance: бок куба +1.0 не проходив цилиндром с соседней клетки; лестница `max_step_up` 0.35 ходит; низ рампы проходим, бок — solid из `build_terrain_side_faces`. Y под ногами всё ещё `SurfaceQuery.sample` (клин как опора — [[feat: Stacked surfaces caves and basements]]).

Spec: `docs/superpowers/specs/2026-09-01-bake-terrain-collision-solids-design.md`

## Resolution

`bake_collision_world` печёт заборы и грани `build_terrain_side_faces` в те же `FenceSolid`. Куб +1.0 блокирует цилиндр сбоку; лестница ≤ 0.35 ходит; низ рампы открыт, бок — стена. Play и `run_input_sequence` передают живую карту; без указателя — только заборы. Стоячий Y по-прежнему `SurfaceQuery.sample` ([[feat: Stacked surfaces caves and basements]]). Заборы ниже 0.35 на integrate больше не блокируют (делят skip с террейном); Mini 0.45 — да.

Проверка: `.\build\tests\rat_tests.exe "[collision]"` и `"[player][surface]"` / `"[player][edge]"`; в Play — обойти куб с соседней клетки, подняться по ступеньке 0.25.

## Bugs found

none.

