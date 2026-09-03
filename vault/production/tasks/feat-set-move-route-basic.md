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

Acceptance: `CommandOp::SetMoveRoute` + parse/serialize `route[]` (`move` N/E/S/W, `wait`, `turn`); runtime overlay `event_id → {tile, facing}`; interpreter yield как Wait; probe коллизии (игрок / blockers / fences); markers из overlay. Target только this; always wait-until-done. Не Event touch, не party follow, не pathfinding, не второй VM.

Origin: [[research: Set Move Route]]. Не Sprint 10 (runtime вне DoD).
