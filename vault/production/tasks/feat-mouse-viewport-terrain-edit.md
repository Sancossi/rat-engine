---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 6
due:
tags: [task]
---

# feat: Mouse viewport terrain edit

Intent: мышью во вьюпорте ставить куб height-grid и пресет ребра (Mini 0.45 / Full 1.6 / Remove), не только ImGui N/E/S/W. Пикл клетки — тот же unproject, что [[feat: Mouse viewport map edit]]. Undo — [[feat: Undo height-grid and edge edits]].

Acceptance: клик по клетке в режиме Place cube поднимает flat tile на +1.0; режим Fence ставит/снимает ребро выбранного направления+пресета; ramp не принимает куб; `WantCaptureMouse`; тест пикла без окна.

Origin: [[feat: Edit and greybox edge walls]]

Depends: [[feat: Mouse viewport map edit]], [[feat: Undo height-grid and edge edits]]. Взято в [[Sprint 6 — Viewport map edit]].
