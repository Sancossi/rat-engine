---
type: task
area: Engine
status: In progress
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: MGS3 ladder climb

Intent: камерный climb (проекция WASD на грань) ощущается плохо. Переделать лестницу по **MGS3**: отдельный Climb на рельсе; пока Climb камера встаёт **сзади**, лестница вверх экрана, «вперёд» = вверх. Snap к центру; сход сверху на пол / снизу на подход.

Acceptance: Interact лицом к лестнице (снизу или сверху) — mount, проход мимо не цепляет. Пока Climb: камера сзади, лестница вверх экрана, вперёд/W = вверх, назад/S = вниз, A/D не двигают. Jump игнорируется. С `y_hi`+вверх — шаг на пол той же клетки; с `y_lo`+вниз — сход на подход. Ходьба в Play — camera-aligned ([[feat: Camera-aligned walk]]); на Climb камера локается так, что вперёд = вверх.

Spec: `docs/superpowers/specs/2026-09-02-mgs3-ladder-climb-design.md`
Plan: `docs/superpowers/plans/2026-09-02-mgs3-ladder-climb.md`
Depends: [[feat: Camera-aligned walk]]
Origin: chat 2026-09-02 (камера-climb плохо). Replaces feel of [[feat: Ladder toward-climb, jump grab, jump off]]. Absorbs [[feat: Step off ladder onto same-tile floor]].
Follow-up: [[feat: Step off ladder onto same-tile floor]] (close after this review).

## Resolution

## Bugs found
