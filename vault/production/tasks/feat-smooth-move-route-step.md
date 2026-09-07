---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Smooth move-route step

Intent: NPC по `set_move_route` телепортируется с клетки на клетку. Нужен плавный шаг world xz со скоростью игрока, маршрут по-прежнему по сетке.

Acceptance: за один кадр `EventOverlay.tile` не прыгает; live xz движется к центру dest с `PlayerBody.speed`; по прибытии snap + `tile = dest`. Markers / Action AABB без volume следуют live xz. Не `integrate_player`, не JSON speed, не трогать `yard_walker` JSON.

Origin: playtest после [[feat-grey-yard-move-route|feat: Grey yard move route]]; [[feat-set-move-route-basic|feat: Set Move Route (basic)]].

## Resolution

`set_move_route` Move интерполирует live xz к центру dest со `PlayerBody.speed`; `tile` коммитится по прибытии. Occupancy (`dest_blocked`) только на старте шага — начатый lerp не зависает между клетками. Verify: Play, `yard_walker` на grey_yard идёт плавно, не телепортируется; если игрок займёт dest после старта шага, NPC доходит. `.\build\tests\rat_tests.exe "[route],[quest]"`. Review: Approved.

## Bugs found

none.
