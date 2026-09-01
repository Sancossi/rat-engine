---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Input action mapping

Intent: слой Input по [[Architecture]] (raw → actions) в духе GPP Command, лёгкая форма — GLFW только в adapter, геймплей читает кадр действий. Контекст: сверка [[Game Programming Patterns]] (2026-08-31). Ребинд и геймпад — не в этой карточке.

Acceptance: `InputFrame` (move, jump, interact, toggle mode, hot-apply, cycle camera); Play/Edit gating как сейчас; headless тест без окна.

## Resolution

`map_input_frame` / `InputButtons` / `InputFrame` в `rat_core`. GLFW только в `sample_editor_buttons`. Play/Edit gating как в editor. F3 добавлен как `debug_snapshot` (для snapshot-карточки). Тесты: `[input]`.

## Bugs found

none.
