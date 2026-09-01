---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Bake height-grid and ramps to collision solids

Intent: после [[edge-walls-passable-from-adjacent-side]] подключить кубы height-grid и рампы к тому же `CollisionWorld`, что и заборы. Edit-представление не меняется; bake на load/hot-apply. Origin: collider design (chat).

Acceptance: бок куба не проходив цилиндром с соседней клетки; рампа — клин-solid (верх walkable); `SurfaceQuery.sample` не обязателен для шага, если опора берётся из solids. Пещеры/слои — [[feat: Stacked surfaces caves and basements]], не эта карточка.
