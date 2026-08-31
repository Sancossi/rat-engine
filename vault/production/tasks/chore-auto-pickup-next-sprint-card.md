---
type: task
area: Production
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: auto-pickup next sprint card

Intent: агент не ждёт «следующая задача», когда текущая карточка спринта закрыта.

Acceptance: alwaysApply rule + строка в [[How we work]].

Origin: chat 2026-08-31.

## Resolution

Rule `.cursor/rules/pickup-next-sprint-card.mdc`. После `Done` — следующая карточка текущего спринта (`In progress` + implementer). Одна ветка, без параллельных implementers.

## Bugs found

none.
