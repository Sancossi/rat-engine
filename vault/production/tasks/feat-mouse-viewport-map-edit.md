---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Mouse viewport map edit

Intent: в Edit править карту мышью во вьюпорте (выбор/перенос/постановка blockers и events на ground plane), а не только слайдерами ImGui. Сейчас `glfwGetMouse` нет; GDD уже требует гизмо. Play мышь для геймплея не берёт. Клики не красть при `WantCaptureMouse`. Пикл/unproject — в `rat_core`, чтобы тест был без окна. Undo — [[feat: Edit undo/redo command stack]], не эта карточка. Контекст: [[ADR-006 Edit-in-playmode author loop]], [[GDD]].

Acceptance: клик выбирает ближайший event/blocker; drag двигает по XZ земли; клик по пустой клетке ставит объект активного инструмента; inspector синхронизируется; высота клетки — follow-up к [[S3: Elevation rendering and Edit tools]].
