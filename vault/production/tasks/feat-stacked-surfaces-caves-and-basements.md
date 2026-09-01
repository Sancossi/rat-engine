---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Stacked surfaces caves and basements

Intent: несколько walkable поверхностей в одной XZ (мост, пещера, подвал, этаж над этажом). Sprint 3 это явно вынес: [[Sprint 3 — Height-grid traversal]]. `SurfaceQuery::sample` уже отдаёт `surface_id`, но хранение однослойное.

Acceptance (позже): под одной клеткой два пола; цилиндр стоит на опоре в диапазоне Y, потолок — solid сверху; не проваливается «сквозь» верхний пол на нижний без дыры. Origin: chat (collider design). Depends: [[edge-walls-passable-from-adjacent-side]] (collision bodies). Related: [[feat: Field physics puzzles]], [[S3: Height-grid map schema and ground query]].
