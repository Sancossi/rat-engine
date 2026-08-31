---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: undo set_events contract test polish

Intent: minor из re-review [[feat: Edit undo/redo command stack]] (Approved): кейс `set_events` clears them вызывает `set_events` уже после acknowledge. Нужна проверка wipe, пока `active_message` ещё «Hello».

Acceptance: `[events]` падает, если `set_events` при открытом диалоге оставляет message/lock.

Origin: [[feat: Edit undo/redo command stack]]
