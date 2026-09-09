---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Ladder toward-climb, jump grab, jump off

Intent: лестница: вверх/вниз по **камере** (к лестнице на экране = +Y). Ходьба вне лестницы — world-aligned. Overlap цепляет. Прыжок на лестнице — спрыгнуть (отскок от грани). Не Interact.

Acceptance: камера смотрит на лестницу → W вверх, S вниз; лестница справа → D вверх. Overlap latch. Jump — XZ от грани + hop + lockout.

Origin: chat 2026-09-02 (переделка climb). Related: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]], [[feat-step-off-ladder-onto-same-tile-floor|feat: Step off ladder onto same-tile floor]].

Spec: `docs/superpowers/specs/2026-09-02-ladder-jump-off-design.md`
Plan: `docs/superpowers/plans/2026-09-02-ladder-jump-off.md`
Follow-up: [[feat-drive-player-physics-from-locomotion-fsm|feat: Drive player physics from locomotion FSM]], [[feat-mgs3-ladder-climb|feat: MGS3 ladder climb]]

## Resolution

Climb up/down follows the camera (`climb_move`); walk stays world-aligned. Overlap latches. Jump on a ladder bounces off the face (hop 4, nudge 0.6, lockout 0.20). Jump-grab: leftover air buffer latches; only a latched buffer or jump press while overlapping bounces. Verify: `.\build\tests\rat_tests.exe "[unit][player]"`.

## Bugs found

Leftover air-jump buffer bounced on first ladder overlap instead of grabbing. Fixed in this slice (`was_climbing` + `latch_climb`).
