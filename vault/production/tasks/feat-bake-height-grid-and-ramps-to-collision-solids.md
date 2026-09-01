---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Bake height-grid and ramps to collision solids

Intent: после [[edge-walls-passable-from-adjacent-side]] подключить кубы height-grid и рампы к тому же `CollisionWorld`, что и заборы. Edit-представление не меняется; bake на load/hot-apply. Origin: collider design (chat).

Acceptance: бок куба +1.0 не проходив цилиндром с соседней клетки; лестница `max_step_up` 0.35 ходит; низ рампы проходим, бок — solid из `build_terrain_side_faces`. Y под ногами всё ещё `SurfaceQuery.sample` (клин как опора — [[feat: Stacked surfaces caves and basements]]).

Spec: `docs/superpowers/specs/2026-09-01-bake-terrain-collision-solids-design.md`

