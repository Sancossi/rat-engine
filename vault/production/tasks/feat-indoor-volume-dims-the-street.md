---
type: task
area: Engine
status: In progress
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

Origin: chat 2026-09-04 (Sprint 13, затенение улицы и домов изнутри дома). Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[feat: Grey yard layout pass]], [[ADR-003 Ortho pixel-stable camera]].

## Resolution

## Bugs found
