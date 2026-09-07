---
type: sprint
status: Done
dates: 2026-09-04/2026-09-18
goal: Grey yard map pass — bridges, multi-side ramps, indoor dim, readable yard, Russian copy
current: false
tags: [sprint]
---

# Sprint 13 — Grey yard map pass

Цель: тем же `grey_yard` сделать двор читаемым как мастерская: мосты над землёй, рампы с разных сторон, затемнение улицы из дома, русские тексты. Не вторая JSON-карта.

DoD:

- Мост: сверху идти, под ним пройти по земле
- Рампы ставятся с кликнутой грани; на платформу можно зайти с двух сторон (две рампы)
- В доме улица/фасады тускнеют
- Двор читается как зоны: земля (квест cog), лофт, gantry, дом/мост если появятся на карте
- Все `show_text` на `grey_yard` на русском
- Apprentice после switch 2 не твердит про недобытый cog
- Headless cog e2e / `[quest]` зелёные

Вне скоупа: successor map, field physics, меши, scare-blocker, i18n UI редактора, [[event-graph-self-pin-drag-stays-armed|Event graph self-pin drag stays armed]].

Порядок:

- [[feat-place-ramp-from-clicked-tile-edge|feat: Place ramp from clicked tile edge]] → [[feat-walkable-bridges-over-open-ground|feat: Walkable bridges over open ground]] → [[feat-indoor-volume-dims-the-street|feat: Indoor volume dims the street]] → [[feat-grey-yard-layout-pass|feat: Grey yard layout pass]] → [[feat-grey-yard-russian-texts|feat: Grey yard Russian texts]] → [[feat-apprentice-post-quest-page|feat: Apprentice post-quest page]]

Roadmap: [[content-vertical-slice|Content vertical slice]]

## Итог

DoD выполнен 2026-09-04. Place ramp с кликнутой грани; мосты `floor_slabs` с проходом снизу; indoor volumes schema 4 затемняют улицу из дома; `grey_yard` — двор / лофт / gantry + дом и мост; все `show_text` по-русски; ученик после switch 2 не охотится за шестернёй. `[quest]` зелёный.

## Bugs found

на закрытии / плейтест: [[cannot-climb-onto-grey-yard-bridge|Cannot climb onto grey_yard bridge]]. Follow-up: [[feat-voxel-3d-terrain-and-rotating-ramps|feat: Voxel 3D terrain and rotating ramps]]. Южный обход ящиков чинили в [[feat-grey-yard-layout-pass|feat: Grey yard layout pass]] до Done. [[event-graph-self-pin-drag-stays-armed|Event graph self-pin drag stays armed]] остаётся в бэклоге.
