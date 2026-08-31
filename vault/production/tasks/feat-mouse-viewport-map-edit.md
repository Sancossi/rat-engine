---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Mouse viewport map edit

Intent: в Edit править карту мышью во вьюпорте (выбор/перенос/постановка blockers и events на ground plane), а не только слайдерами ImGui. Сейчас `glfwGetMouse` нет; GDD уже требует гизмо. Play мышь для геймплея не берёт. Клики не красть при `WantCaptureMouse`. Пикл/unproject — в `rat_core`, чтобы тест был без окна. Undo — [[feat: Edit undo/redo command stack]], не эта карточка. Контекст: [[ADR-006 Edit-in-playmode author loop]], [[GDD]].

Acceptance: клик выбирает ближайший event/blocker; drag двигает по XZ земли; клик по пустой клетке ставит объект активного инструмента; inspector синхронизируется. Высота клетки и рёбра — [[feat: Mouse viewport terrain edit]].

Depends: none. Next: [[feat: Undo height-grid and edge edits]]. Взято в [[Sprint 6 — Viewport map edit]].
