---
type: task
area: Engine
status: In progress
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: locomotion classify review polish

Intent: minor из ревью [[feat: Player locomotion FSM]] (Approved): тест grounded + `vertical_speed > 0` не даёт Jump; цикл до апекса в `[loco]` имеет кап итераций.

Acceptance: `[loco]` падает, если classify отдаёт Jump на земле; while не может зависнуть.

Origin: [[feat: Player locomotion FSM]]
