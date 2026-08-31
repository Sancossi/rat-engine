---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Headless input sequence probe

Intent: прогон N фиксированных шагов (120 Hz) с заранее заданными `PlayerFrameInput` без GLFW — репро «прыгнул и провалился» для агента. Зависит от кадра ввода ([[feat: Input action mapping]]) или текущего `PlayerFrameInput`. На каждом шаге опционально snapshot. Контекст: [[Agent Debug]], [[ADR-010 Catch2 unit and headless mechanics tests]].

Acceptance: helper/CLI: последовательность входов → конечный xyz/`GameState`; один Catch2-кейс на grey_yard; расхождение ломает тест, не требует скрина.
