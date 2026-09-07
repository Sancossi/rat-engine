---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Edit undo/redo command stack

Intent: GPP Command для редактора (не для ввода): обратимые правки blockers/events в Edit по [[ADR-006 Edit-in-playmode author loop]]. Сейчас мутация карты необратима.

Acceptance: undo/redo для place/move/delete blocker и event; стек сбрасывается на hot-apply/load; Play-режим историю не пишет.

## Resolution

`EditHistory` + `EditCommand` in `rat_core`. Place/delete/grid-move of blockers and events go through `execute`; blocker mutations do not call `set_events` (autorun/dialog survive Edit). Ctrl+Z/Y in Edit; `clear()` on successful hot-apply. Verify: `.\build\tests\rat_tests.exe "[edit]"`.

Follow-up: [[feat-undo-grow-shrink-and-field-edits|feat: undo grow/shrink and field edits]]
Follow-up: [[chore-undo-set-events-contract-test-polish|chore: undo set_events contract test polish]]

## Bugs found

none (продуктовых). Important `set_events` на blocker-edit — закрыт в `2b8b23d`. Minor ревью: контрактный тест не ловит wipe при живом сообщении — см. follow-up.

