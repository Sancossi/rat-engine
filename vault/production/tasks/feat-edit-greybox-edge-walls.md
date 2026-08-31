---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 5
due:
tags: [task]
---

# feat: Edit and greybox edge walls

Intent: авторинг и видимость рёбер в Edit/greybox плюс пример на `grey_yard`. Пресеты мини 0.45 / полная 1.6. Мышь во вьюпорте и undo height/edges — не эта карточка ([[feat: Mouse viewport map edit]], [[feat: Edit undo/redo command stack]]). Origin: [[feat: Grid edge barriers]].

Acceptance: ImGui N/E/S/W + пресет на выбранной клетке; greybox рисует стенку ребра; в `grey_yard` есть куб, мини-ребро и полное ребро; save/load roundtrip.

Depends: [[feat: Grid edge barriers]]. Взято в [[Sprint 5 — Terrain and edge walls]].

## Resolution

ImGui N/E/S/W + Mini 0.45 / Full 1.6 on the selected elevation tile; Remove edge. Greybox draws thin fence quads from `owner_top` to `owner_top + height`. `grey_yard` keeps the z=8 cube strip and ships east fences at (4,6) mini / (6,6) full with save/load roundtrip. `upsert_map_ramp` drops fences on that tile.

Verify: `.\build\tests\rat_tests.exe "[height_edit]"`, `"[terrain]"`, `"[map]"`.

## Bugs found

none.
