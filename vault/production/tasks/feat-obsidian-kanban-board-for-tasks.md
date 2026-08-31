---
type: task
area: Production
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Obsidian kanban board for tasks

Intent: канбан поверх файлов в `production/tasks/` (YAML), без второй базы карточек. Две проекции: все задачи по `status` и те же задачи по `sprint`.

Acceptance: в vault открывается доска по статусу; отдельно — разрез по спринтам; карточка ведёт на заметку; агент по-прежнему Grep'ает frontmatter.

Chat: «Хочу канбан доску для задач в obsidian»; уточнение: все задачи + вид по спринтам (2026-08-31).

Сделано: [[Task board]] (`vault/production/Task board.base`), embed на [[Tasks]]. Виды: Current sprint, In progress, Board, Sprints, Table.

## Resolution

Base в git. Люди смотрят **Current sprint** и **In progress**; YAML `status` = колонка.

## Bugs found

none.
