---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Headless input sequence probe

Intent: прогон N фиксированных шагов (120 Hz) с заранее заданными `PlayerFrameInput` без GLFW — репро «прыгнул и провалился» для агента. Зависит от кадра ввода ([[feat: Input action mapping]]) или текущего `PlayerFrameInput`. На каждом шаге опционально snapshot. Контекст: [[Agent Debug]], [[ADR-010 Catch2 unit and headless mechanics tests]].

Acceptance: helper/CLI: последовательность входов → конечный xyz/`GameState`; один Catch2-кейс на grey_yard; расхождение ломает тест, не требует скрина.

## Resolution

`run_input_sequence` в `rat_core` (`rat/input_sequence.hpp`): N шагов 120 Hz, `InputFrame`, без GLFW. Опционально snapshot на шаг. Catch2 `[probe]` на `grey_yard` (сначала снять autorun intro). CLI не делали (YAGNI). Ревью: Approved.

Follow-up: [[chore: input sequence probe review polish]]

## Bugs found

none (продуктовых). Minor ревью: тихий IO snapshot, Edit без snap-to-ground, общий temp теста — см. follow-up.
