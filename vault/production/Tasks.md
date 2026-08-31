---
type: index
tags: [task]
---

# Tasks

Одна задача = один файл в `production/tasks/`. Поля: `type: task`, `area`, `status`, `task_type`, `sprint`, `due`.

Статусы: `Not started` | `In progress` | `Done` | `Archived`.
Area: `Engine` | `Game` | `Production`.
Type: `Feature` | `Bug` | `Chore` | `Research`.

Агент: Grep `status: In progress` или `sprint: Sprint 3` в `vault/production/tasks/`.

```dataview
TABLE area, status, task_type, sprint
FROM "production/tasks"
WHERE type = "task"
SORT status ASC, file.name ASC
```
