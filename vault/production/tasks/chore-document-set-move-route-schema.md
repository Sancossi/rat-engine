---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 11
due:
tags: [task]
---

# chore: Document set_move_route in map schema

Intent: `docs/schemas/map-event.schema.md` enumerates commands (`play_se` is there); `set_move_route` + nested `route[]` нет.

Acceptance: одна строка в таблице Commands: `op`, `through`, `route[]` (`move`/`wait`/`turn`). Не второй research.

Origin: [[feat: Set Move Route (basic)]] (review Minor).
