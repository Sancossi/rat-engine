---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Event why-not-fired

Intent: для event id вернуть причину, почему page не стартовала на этом кадре: `wrong_page` / `conditions` / `height` / `not_overlapping` / `out_of_action_range` / `input_blocked` / `already_running` / `ok`. `event_inspect` сейчас только подписи для ImGui, не runtime. Контекст: [[Agent Debug]], [[Event System]].

Acceptance: функция в `rat_core` + строка в snapshot/inspector; тесты на height mismatch и failed switch.

Origin: [[Sprint 4 — Refactoring and AI workflow]]
Follow-up: [[feat: Debug snapshot why-not for selected event]]
Follow-up: [[feat: why-not ok should match start locks]]

## Resolution

`EventRuntime::why_not_fired` + `event_why_not_name` в `rat_core`. Коды: `wrong_page` / `conditions` / `height` / `not_overlapping` / `out_of_action_range` / `input_blocked` / `already_running` / `ok`. Inspector: `Why not:` у выбранного event. Snapshot: список `event_why_not` на все events; primary — первое overlapping. Тесты: `rat_tests.exe "[why]"`. Review: Approved.

## Bugs found

- Primary snapshot reason ≠ selection → [[feat: Debug snapshot why-not for selected event]]
- `ok` не совпадает с `try_start_*` (autorun_lock, touch already inside, parallel cap) → [[feat: why-not ok should match start locks]]
