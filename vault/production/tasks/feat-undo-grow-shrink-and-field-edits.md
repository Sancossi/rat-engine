---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: undo grow/shrink and field edits

Intent: place/move/delete уже в [[feat: Edit undo/redo command stack]]; grow/shrink AABB, jumpable/vertical range и правки текста/страниц события всё ещё необратимы (YAGNI той карточки).

Acceptance: те же `EditHistory` команды (или replace-at-index) для resize/jumpable/page-text; Play по-прежнему не пишет стек.

Origin: [[feat: Edit undo/redo command stack]]

## Resolution

Replace-at-index: `make_replace_blocker_command` / `make_replace_event_command` capture previous on apply. Grow/shrink/snap, jumpable + Base Y/Top Y, and event trigger/switch/Show Text/Place on tile go through `run_history`. Blocker replace reports `mutates_events == false`. ImGui `InputFloat` / `InputTextMultiline` / `InputInt` push one command on `IsItemDeactivatedAfterEdit`; buttons and checkboxes execute immediately. Play does not write the stack (Edit UI only). Height-grid still out of scope. Live field-edit origin is discarded (not committed) on Load, mode change, and at the start of `run_history`, so a later `IsItemDeactivatedAfterEdit` cannot restore a foreign previous onto a new map. Review: Approved.

Verify: `rat_tests "[edit]"` plus `[blocker_edit]` / `[event_edit]`; Ctrl+Z after Grow +X or Show Text in Edit restores the previous value. Load while a Base Y / Show Text widget is still active must not put an old-map blocker on the undo stack.

## Bugs found

none.
