---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Edit and greybox edge walls

Intent: авторинг и видимость рёбер в Edit/greybox плюс пример на `grey_yard`. Пресеты мини 0.45 / полная 1.6. Мышь во вьюпорте и undo height/edges — не эта карточка ([[feat: Mouse viewport map edit]], [[feat: Edit undo/redo command stack]]). Origin: [[feat: Grid edge barriers]].

Acceptance: ImGui N/E/S/W + пресет на выбранной клетке; greybox рисует стенку ребра; в `grey_yard` есть куб, мини-ребро и полное ребро; save/load roundtrip.

Depends: [[feat: Grid edge barriers]].
