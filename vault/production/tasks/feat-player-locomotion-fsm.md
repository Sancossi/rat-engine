---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Player locomotion FSM

Intent: GPP State — именованные состояния Idle/Walk/Jump/Fall для анимации по [[ADR-009 RE-like segmented character hierarchy]]. `JumpState` остаётся физикой, не FSM. Брать после того, как понадобится клип/процедурная анимация, не раньше.

Acceptance: переходы детерминированы и покрыты headless-тестом; физика прыжка не ломается.
