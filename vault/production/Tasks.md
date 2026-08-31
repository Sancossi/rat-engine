---
type: index
tags: [task]
---

# Tasks

Одна задача = один файл в `production/tasks/`. Поля: `type: task`, `area`, `status`, `task_type`, `sprint`, `due`.

Статусы: `Not started` | `In progress` | `Done` | `Archived`.
Area: `Engine` | `Game` | `Production`.
Type: `Feature` | `Bug` | `Chore` | `Research`.

Канбан: [[Task board]] (виды **Current sprint** / **In progress** / Board / Sprints / Table).

- **Current sprint** — канбан `sprint: Sprint 4` (колонки Not started / In progress / Done). При смене `current: true` обновить фильтр `sprint == "Sprint N"` в `Task board.base`.
- **In progress** — только карточки `status: In progress` (implementer и ещё не Approved review). Пока агент работает, статус не `Done`.

Правка свойства в Base пишет YAML заметки.

Агент: Grep `status: In progress` или `sprint: Sprint 4` в `vault/production/tasks/`.

![[Task board.base]]
