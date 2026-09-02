---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Edit hold-drag brush

Intent: в Edit зажать ЛКМ и вести — активный **клеточный** инструмент красится по каждой клетке под курсором (Place cube, slab, высота), один undo на stroke.

Acceptance: drag crossing N tiles applies N mutations as one `EditHistory` group; release ends the stroke; Play не пишет. Не стены по ребру (это [[feat: Edit paint clicked edge]]).

Origin: [[feat: Sims-like edit brush and edge paint]]. Depends: [[feat: Mouse viewport terrain edit]]. Взято в [[Sprint 8 — Play feel and authoring]].
