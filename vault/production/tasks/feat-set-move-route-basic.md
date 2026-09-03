---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Set Move Route (basic)

Intent: NPC ходит по сетке без physics-ящиков. Формат и тик уже решены в [[research: Set Move Route]].

Acceptance: `CommandOp::SetMoveRoute` + parse/serialize `route[]` (`move` N/E/S/W, `wait`, `turn`); runtime overlay `event_id → {tile, facing}`; interpreter yield как Wait; probe коллизии (игрок / blockers / fences); markers из overlay. Target только this; always wait-until-done. Foreground (Action/Autorun/PlayerTouch): занятость игрока = through (залоченный игрок не сойдёт с dest); blockers/fences блокируют без `through: true`; Parallel ждёт на занятой. Overlay-caused overlap не стартует PlayerTouch — только когда игрок сам заходит в live bounds; страница PlayerTouch может содержать `set_move_route`. Overlay-клетка побеждает; tile+volume live bounds = volume, сдвинутый как `translate_event_on_grid`; volume-only skip+warn. Не Event touch, не party follow, не pathfinding, не второй VM.

Origin: [[research: Set Move Route]]. Не Sprint 10 (runtime вне DoD).
