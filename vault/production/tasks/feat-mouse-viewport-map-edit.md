---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Mouse viewport map edit

Intent: в Edit править карту мышью во вьюпорте (выбор/перенос/постановка blockers и events на ground plane), а не только слайдерами ImGui. Сейчас `glfwGetMouse` нет; GDD уже требует гизмо. Play мышь для геймплея не берёт. Клики не красть при `WantCaptureMouse`. Пикл/unproject — в `rat_core`, чтобы тест был без окна. Undo — [[feat: Edit undo/redo command stack]], не эта карточка. Контекст: [[ADR-006 Edit-in-playmode author loop]], [[GDD]].

Acceptance: клик выбирает ближайший event/blocker; drag двигает по XZ земли; клик по пустой клетке ставит объект активного инструмента; inspector синхронизируется. Высота клетки и рёбра — [[feat: Mouse viewport terrain edit]].

Depends: none. Next: [[feat: Undo height-grid and edge edits]]. Взято в [[Sprint 6 — Viewport map edit]].

## Resolution

В Edit клик/drag/place blockers и events идут через `unproject_to_ground_plane` + `resolve_viewport_click` в `rat_core` (тест без окна). Луч строится инверсией живых 4×4 `view`/`proj` (как `bx::mtxLookAt` Left / greybox), без `-world_z`. `WantCaptureMouse` не крадёт клики; Play мышью карту не правит. Undo place/move — существующий `EditHistory`.

Проверка: `.\build\tests\rat_tests.exe "[viewport_edit]"`; в Edit — Select / Place blocker / Place event, центр и off-center клик на TopDown.

## Bugs found

none.
