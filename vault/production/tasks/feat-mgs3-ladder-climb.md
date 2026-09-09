---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: MGS3 ladder climb

Intent: камерный climb (проекция WASD на грань) ощущается плохо. Переделать лестницу по **MGS3**: отдельный Climb на рельсе; пока Climb камера встаёт **сзади**, лестница вверх экрана, «вперёд» = вверх. Snap к центру; сход сверху на пол / снизу на подход.

Acceptance: Interact лицом к лестнице (снизу или сверху) — mount, проход мимо не цепляет. Пока Climb: камера сзади, лестница вверх экрана, вперёд/W = вверх, назад/S = вниз, A/D не двигают. Jump игнорируется. С `y_hi`+вверх — шаг на пол той же клетки; с `y_lo`+вниз — сход на подход. Ходьба в Play — camera-aligned ([[feat-camera-aligned-walk|feat: Camera-aligned walk]]); на Climb камера локается так, что вперёд = вверх.

Spec: `docs/archive/cpp/superpowers/specs/2026-09-02-mgs3-ladder-climb-design.md`
Plan: `docs/archive/cpp/superpowers/plans/2026-09-02-mgs3-ladder-climb.md`
Depends: [[feat-camera-aligned-walk|feat: Camera-aligned walk]]
Origin: chat 2026-09-02 (камера-climb плохо). Replaces feel of [[feat-ladder-toward-climb-jump-grab|feat: Ladder toward-climb, jump grab, jump off]]. Absorbs [[feat-step-off-ladder-onto-same-tile-floor|feat: Step off ladder onto same-tile floor]].
Follow-up: [[feat-step-off-ladder-onto-same-tile-floor|feat: Step off ladder onto same-tile floor]] (closed here — top + up onto same-tile slab).
Follow-up: [[climb-camera-locks-on-far-side]]
Follow-up: [[feat-smooth-camera-turn|feat: Smooth camera turn]]
Follow-up: [[walk-through-ladder]]

## Resolution

Лестница — Interact-рельс: проход мимо не цепляет; E в зоне (overlap / подход снизу / плита сверху) сажает на центр объёма. Пока Climb камера greybox сзади, лестница вверх экрана, W/S = ±Y, A/D и Jump игнорируются. Сход: `y_hi`+вверх на support той же клетки, `y_lo`+вниз на подход. Action на 0-tick display frame (144 vs 120 Hz) тоже маунтит. Проверка: Play у East-лестницы — E, W вверх, Jump ничего, сверху на плиту; `.\build\tests\rat_tests.exe "[unit][player],[unit][sim],[unit][camera]"`.

## Bugs found

- [[climb-camera-locks-on-far-side]] — playtest 2026-09-02, подход с `(3, 5)` к East-лестнице `(2, 4)`: камера с дальнего края стены.
- [[walk-through-ladder]] — playtest 2026-09-02 evening: сквозь лестницу можно пройти; нужна стена на грани.
