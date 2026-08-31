---
type: task
area: Production
status: Not started
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: task status In review

Intent: на доске отличить «пишут код» от «ждёт Approved» и от «стоп, нужен человек / зависимость».

Acceptance: YAML `status` и колонки [[Task board]] совпадают с ритуалом в [[How we work]]; агентские правила (pickup / vault-status) читают тот же enum.

Предлагаемый enum задач: `Not started` | `In progress` | `In review` | `Blocked` | `Done` | `Archived`.

Origin: [[Sprint 4 — Refactoring and AI workflow]] (сбивка статусов при последовательном pickup).
