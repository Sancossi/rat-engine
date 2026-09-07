---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: locomotion classify review polish

Intent: minor из ревью [[feat-player-locomotion-fsm|feat: Player locomotion FSM]] (Approved): тест grounded + `vertical_speed > 0` не даёт Jump; цикл до апекса в `[loco]` имеет кап итераций.

Acceptance: `[loco]` падает, если classify отдаёт Jump на земле; while не может зависнуть.

Origin: [[feat-player-locomotion-fsm|feat: Player locomotion FSM]]

## Resolution

`[loco]` locks grounded + `vertical_speed > 0` as Idle (zero move) or Walk (non-zero axis), never Jump — classify already required `!grounded` before Jump; the case fails if airborne-vs is checked first. Apex integrate loop is capped at 600 frames and `REQUIRE`s the cap so Fall is not skipped on hang. Jump physics and `locomotion_from` product unchanged. Review: Approved.

Verify: `.\build\tests\rat_tests.exe "[loco]"`.

## Bugs found

none.
