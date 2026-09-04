---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 13
due:
tags: [task]
---

# feat: Indoor volume dims the street

Intent: зайдя в дом, улица и фасады должны **затемняться**, интерьер читаться. Не GI и не смена камеры. Greybox: игрок в authored indoor-объёме → outdoor fill/clear темнее (дом снаружи тоже), пока ноги в объёме.

Acceptance:

- В JSON карты список indoor volumes (AABB: xz + y_lo/y_hi или tile set + высота). Schema + parse/dump.
- Play/Edit greybox: если цилиндр игрока пересекает volume — dim outdoor terrain/slabs/blockers **снаружи** объёма; внутри объёма яркость не падает (или отдельный indoor tint). Вышел на улицу — как сейчас.
- Unit: «игрок в AABB → indoor true». Не PBR, не stencil portal, не вторая карта.

Origin: chat 2026-09-04 (Sprint 13, затенение улицы и домов изнутри дома). Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[feat: Grey yard layout pass]], [[ADR-003 Ortho pixel-stable camera]]. Follow-up: [[feat: Grey yard layout pass]] — inset indoor AABB к внутренним тайлам (центр квада на грани volume остаётся ярким).

## Resolution

Schema 4: `indoor_volumes` (AABB xz + `y_lo`/`y_hi`). Loader 1–3 без volumes. `player_inside_indoor_volume` — цилиндр 1.6 vs AABB. Greybox: indoor → outdoor fill ×0.4, внутри объёма без dim. На `grey_yard` volumes нет (layout pass). Review: Approved.

Verify: unit `[map],[collision],[terrain]`; playtest dim появится после authored volume на карте.

## Bugs found

none.
