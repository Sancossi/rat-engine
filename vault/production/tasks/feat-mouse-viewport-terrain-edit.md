---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Mouse viewport terrain edit

Intent: мышью во вьюпорте ставить куб height-grid и пресет ребра (Mini 0.45 / Full 1.6 / Remove), не только ImGui N/E/S/W. Пикл клетки — тот же unproject, что [[feat-mouse-viewport-map-edit|feat: Mouse viewport map edit]]. Undo — [[feat-undo-height-and-edge-edits|feat: Undo height-grid and edge edits]].

Acceptance: клик по клетке в режиме Place cube поднимает flat tile на +1.0; режим Fence ставит/снимает ребро выбранного направления+пресета; ramp не принимает куб; `WantCaptureMouse`; тест пикла без окна.

Origin: [[feat-edit-greybox-edge-walls|feat: Edit and greybox edge walls]]

Depends: [[feat-mouse-viewport-map-edit|feat: Mouse viewport map edit]], [[feat-undo-height-and-edge-edits|feat: Undo height-grid and edge edits]]. Взято в [[Sprint 6 — Viewport map edit]].

## Resolution

В Edit клик Place cube поднимает плоскую клетку на +1.0 через `make_place_map_tile_cube_command`; Fence ставит Mini 0.45 / Full 1.6 или Remove на направление из ImGui combo. Пикл — тот же `unproject` / `resolve_viewport_click`; клик по объекту выделяет. Ramp куб не поднимает. `WantCaptureMouse`; Play мышью не правит. Undo — существующий `EditHistory`.

Проверка: `.\build\tests\rat_tests.exe "[viewport_edit]"`; в Edit — Place cube / Fence по клетке, Ctrl+Z.

## Bugs found

none.
