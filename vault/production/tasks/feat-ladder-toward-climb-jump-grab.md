---
type: task
area: Engine
status: In review
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Ladder toward-climb, jump grab, jump off

Intent: лестница: вверх/вниз по **камере** (к лестнице на экране = +Y). Ходьба вне лестницы — world-aligned. Overlap цепляет. Прыжок на лестнице — спрыгнуть (отскок от грани). Не Interact.

Acceptance: камера смотрит на лестницу → W вверх, S вниз; лестница справа → D вверх. Overlap latch. Jump — XZ от грани + hop + lockout.

Origin: chat 2026-09-02 (переделка climb). Related: [[feat: Stacked surfaces caves and basements]], [[feat: Step off ladder onto same-tile floor]].

Spec: `docs/superpowers/specs/2026-09-02-ladder-jump-off-design.md`
Plan: `docs/superpowers/plans/2026-09-02-ladder-jump-off.md`
Follow-up: [[feat: Drive player physics from locomotion FSM]]

## Resolution

## Bugs found
