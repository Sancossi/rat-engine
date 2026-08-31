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

`EditHistory` + `EditCommand` in `rat_core` (no singleton). Place/delete/grid-move of blockers and events go through `execute`; `undo`/`redo` restore the same indices. `EditorApp` owns the stack, records only in Edit UI, applies Ctrl+Z / Ctrl+Y (Ctrl+Shift+Z) via `InputFrame` when ImGui is not capturing keyboard, and `clear()`s on successful hot-apply (covers init load). Height-grid, grow/shrink, and field tweaks stay out of the stack. Verify: `.\build\tests\rat_tests.exe "[edit]"`.

Follow-up: [[feat: undo grow/shrink and field edits]]

## Bugs found

none.

