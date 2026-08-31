---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Undo height-grid and edge edits

Intent: Place cube, ramp и edge fence сейчас мутируют карту в обход `EditHistory` (YAGNI [[feat: undo grow/shrink and field edits]]). После [[Sprint 5 — Terrain and edge walls]] это основной авторский путь — нужен Ctrl+Z/Y как у blockers/events.

Acceptance: Place cube / `upsert_map_ramp` / upsert+remove `edge_barriers` идут через `EditHistory`; Play не пишет стек; `clear()` на hot-apply/load; headless-тест без окна.

Origin: [[feat: Edit and greybox edge walls]]

Depends: none. Next: [[feat: Mouse viewport terrain edit]]. Взято в [[Sprint 6 — Viewport map edit]].
