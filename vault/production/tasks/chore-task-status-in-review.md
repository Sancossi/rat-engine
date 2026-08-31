---
type: task
area: Production
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: task status In review

Intent: на доске отличить «пишут код» от «ждёт Approved» и от «стоп, нужен человек / зависимость».

Acceptance: YAML `status` и колонки [[Task board]] совпадают с ритуалом в [[How we work]]; агентские правила (pickup / vault-status) читают тот же enum.

Origin: [[Sprint 4 — Refactoring and AI workflow]] (сбивка статусов при последовательном pickup).

## Resolution

Task enum is `Not started` | `In progress` | `In review` | `Blocked` | `Done` | `Archived`. [[Task board]] Current sprint has those columns; the In progress *view* shows In progress + In review + Blocked. Implementer stays In progress; parent flips In review after the implementer commit; Done after Approved; Needs fixes returns to In progress. Blocked stops pickup — reason in the card body (includes waiting on the human). Wiki-only / status-only cards may skip In review. Verify: YAML on a live card and columns in `Task board.base`.

## Bugs found

none.
