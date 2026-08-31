---
type: task
area: Engine
status: In progress
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: undo set_events contract test polish

Intent: minor из re-review [[feat: Edit undo/redo command stack]] (Approved): кейс `set_events` clears them вызывает `set_events` уже после acknowledge. Нужна проверка wipe, пока `active_message` ещё «Hello».

Acceptance: `[events]` падает, если `set_events` при открытом диалоге оставляет message/lock.

Origin: [[feat: Edit undo/redo command stack]]

## Resolution

`[events]` now calls `set_events` while `active_message` is still `"Hello"` (no acknowledge). Asserts message and autorun lock are wiped; `set_blockers` still keeps both. Product `EventRuntime::set_events` already cleared them — no product change. After wipe, autorun may fire again on the next `update`.

Verify: `.\build\tests\rat_tests.exe "[events]"`.

## Bugs found

none.
