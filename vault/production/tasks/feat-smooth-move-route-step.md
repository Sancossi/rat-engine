---
type: task
area: Engine
status: In progress
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Smooth move-route step

Intent: NPC по `set_move_route` телепортируется с клетки на клетку. Нужен плавный шаг world xz со скоростью игрока, маршрут по-прежнему по сетке.

Acceptance: за один кадр `EventOverlay.tile` не прыгает; live xz движется к центру dest с `PlayerBody.speed`; по прибытии snap + `tile = dest`. Markers / Action AABB без volume следуют live xz. Не `integrate_player`, не JSON speed, не трогать `yard_walker` JSON.

Origin: playtest после [[feat: Grey yard move route]]; [[feat: Set Move Route (basic)]].
