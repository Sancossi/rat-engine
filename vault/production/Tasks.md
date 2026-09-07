---
type: index
tags: [task]
---

# Tasks

Одна задача = один файл в `production/tasks/`. Поля: `type: task`, `area`, `status`, `task_type`, `sprint`, `due`.

Статусы: `Not started` | `In progress` | `In review` | `Blocked` | `Done` | `Archived`.
Area: `Engine` | `Game` | `Production`.
Type: `Feature` | `Bug` | `Chore` | `Research`.

Канбан: [[Task board]] (виды **Current sprint** / **In progress** / Board / Sprints / Table).

- **Current sprint** — канбан `sprint: Sprint 15` (колонки Not started / In progress / In review / Blocked / Done). Спринт: [[Sprint 15 — Stabilization]].
- **In progress** (вид) — `In progress` (пишут код) / `In review` (ждёт Approved) / `Blocked` (стоп, причина в теле). `Done` только после Approved.

Правка свойства в Base пишет YAML заметки.

Агент: Grep `status: In progress`, `In review`, `Blocked`, или `sprint: Sprint 15` в `vault/production/tasks/`.

![[Task board.base]]

Bugs from `production/bugs` share this board: Open -> Investigating -> Fixed (or Wont Fix). Review uses the separate `review` field.
