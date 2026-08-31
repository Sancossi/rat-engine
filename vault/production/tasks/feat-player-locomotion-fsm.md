---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Player locomotion FSM

Intent: GPP State — именованные состояния Idle/Walk/Jump/Fall для анимации по [[ADR-009 RE-like segmented character hierarchy]]. `JumpState` остаётся физикой, не FSM. Взято в [[Sprint 4 — Refactoring and AI workflow]]: каркас состояний до клипов, чтобы анимация потом подписалась на имена, а не наоборот.

Acceptance: переходы детерминированы и покрыты headless-тестом; физика прыжка не ломается.
