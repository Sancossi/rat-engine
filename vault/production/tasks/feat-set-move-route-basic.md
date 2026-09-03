---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 11
due:
tags: [task]
---

# feat: Set Move Route (basic)

Intent: NPC ходит по сетке без physics-ящиков. Формат и тик уже решены в [[research: Set Move Route]].

Acceptance: `CommandOp::SetMoveRoute` + parse/serialize `route[]` (`move` N/E/S/W, `wait`, `turn`); runtime overlay `event_id → {tile, facing}`; interpreter yield как Wait; probe коллизии (игрок / blockers / fences); markers из overlay. Target только this; always wait-until-done. Foreground (Action/Autorun/PlayerTouch): занятость игрока = through (залоченный игрок не сойдёт с dest); blockers/fences блокируют без `through: true`; Parallel ждёт на занятой. Overlay-caused overlap не стартует PlayerTouch — только когда игрок сам заходит в live bounds; страница PlayerTouch может содержать `set_move_route`. Overlay-клетка побеждает; tile+volume live bounds = volume, сдвинутый как `translate_event_on_grid`; volume-only skip+warn. Не Event touch, не party follow, не pathfinding, не второй VM.

Origin: [[research: Set Move Route]]. Взято в [[Sprint 11 — Set Move Route]]. Не трогать `grey_yard.json` (это [[feat: Grey yard move route]]).

Follow-up: [[chore: Document set_move_route in map schema]], [[feat: Smooth move-route step]]

## Resolution

`CommandOp::SetMoveRoute` + `Command::route` (`RouteStep` Move/Wait/Turn) + `through`. Overlay `EventRuntime::event_overlay`. Interpreter: `command_index` на opcode, `route_index` + `wait_frames`. Foreground: игрок не блокирует шаг; Parallel ждёт; PlayerTouch только от шага игрока. Verify: `.\build\tests\rat_tests.exe "[route]"`. Play: cyan marker двигается после [[feat: Grey yard move route]]. Review: Approved.

## Bugs found

none. Review Minor (нет строки в schema) → [[chore: Document set_move_route in map schema]].
