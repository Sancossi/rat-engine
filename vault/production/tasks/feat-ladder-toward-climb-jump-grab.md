---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Ladder toward-climb, jump grab, jump off

Intent: лестница: к грани +Y, от грани −Y (как сейчас). Overlap всегда цепляет. Прыжок на лестнице — спрыгнуть (отскок от грани). Прыжок с земли/воздуха — обычный hop, чтобы дотянуться до объёма. Не Interact.

Acceptance: climb-ось без изменений. Overlap latch. На лестнице jump — XZ от грани + hop + короткий lockout. Вне лестницы jump без смены правил, кроме lockout после спрыжка.

Origin: chat 2026-09-02 (переделка climb). Related: [[feat: Stacked surfaces caves and basements]], [[feat: Step off ladder onto same-tile floor]].

Spec: `docs/superpowers/specs/2026-09-02-ladder-jump-off-design.md`

## Resolution

## Bugs found
