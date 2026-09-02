---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Drive player physics from locomotion FSM

Intent: сейчас `locomotion_from` только **классифицирует** Idle/Walk/Jump/Fall для анимации; тик всё ещё один `integrate_player_frame_surface` с флагами (лестница, lockout, coyote). Сделать FSM **драйвером**: состояние владеет integrate. Добавить **Climb**. Переходы: Idle/Walk ↔ Jump/Fall; overlap → Climb; jump на лестнице → Fall (bounce); lockout не Climb.

Acceptance: Climb в `locomotion_from` + имена; integrate разбит по состояниям (или явный switch без скрытых `continue`); текущие `[unit][player]` / `[loco]` зелёные. Не смешивать с Interact-to-climb.

Origin: chat 2026-09-02 (уточнение к лестнице). Related: [[feat: Player locomotion FSM]], [[feat: Ladder toward-climb, jump grab, jump off]].

## Resolution

`LocomotionState::Climb` + `JumpState.climbing`. Each jump substep `switch`es Climb (`tick_climb_substep`) vs Idle/Walk/Jump/Fall (`tick_ground_air_substep`). Bounce leaves Climb and runs ballistic; lockout is not Climb. Verify: `.\build\tests\rat_tests.exe "[unit][loco],[unit][player]"`.

## Bugs found

none. Jump-grab buffer bounce was found on [[feat: Ladder toward-climb, jump grab, jump off]] and fixed there.
