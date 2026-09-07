---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Player locomotion FSM

Intent: GPP State — именованные состояния Idle/Walk/Jump/Fall для анимации по [[ADR-009 RE-like segmented character hierarchy]]. `JumpState` остаётся физикой, не FSM. Взято в [[Sprint 4 — Refactoring and AI workflow]]: каркас состояний до клипов, чтобы анимация потом подписалась на имена, а не наоборот.

Acceptance: переходы детерминированы и покрыты headless-тестом; физика прыжка не ломается.

## Resolution

`locomotion_from` / `locomotion_state_name` in `rat_core` (`rat/locomotion.hpp`): pure classify Idle/Walk/Jump/Fall from `JumpState` + `MoveInput`. Names `"Idle"` `"Walk"` `"Jump"` `"Fall"` are the animation contract. Jump gravity/coyote/buffer stay in `JumpState` / `integrate_player_frame_surface`. Verify: `.\build\tests\rat_tests.exe "[loco]"`.

Follow-up: [[chore-locomotion-classify-review-polish|chore: locomotion classify review polish]], [[feat-drive-player-physics-from-locomotion-fsm|feat: Drive player physics from locomotion FSM]]

## Bugs found

none (продуктовых). Minor ревью: нет кейса grounded+vs>0; while до апекса без капа — см. follow-up.
