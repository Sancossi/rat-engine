---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Edit undo/redo command stack

Intent: GPP Command для редактора (не для ввода): обратимые правки blockers/events в Edit по [[ADR-006 Edit-in-playmode author loop]]. Сейчас мутация карты необратима.

Acceptance: undo/redo для place/move/delete blocker и event; стек сбрасывается на hot-apply/load; Play-режим историю не пишет.
