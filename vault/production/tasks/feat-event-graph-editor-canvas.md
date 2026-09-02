---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Event graph editor canvas

Intent: в Edit у выбранного event/page — холст нод (trigger, conditions, commands, branch). Правка графа пишет документ и hot-apply как сейчас список команд.

Acceptance: place/connect/delete nodes; compile on apply; inspector list остаётся fallback. MVP без полного MZ-набора. Не Ruby/JS VM.

Origin: [[feat: Event node graph authoring]]. Depends: [[feat: Event graph model and compile]]. Related: [[s2-event-inspector-pages-stub]]. Взято в [[Sprint 8 — Play feel and authoring]].

Follow-up: [[chore: Event graph join golden and branch edges]]

## Resolution

Edit: ImGui canvas for Show Text / Switch / Branch / Wait (place, connect, delete). Save, Apply edited map, and Compile graph run `compile_event_graph` into `page.commands`. Pages without `graph` keep the inspector list. Play still executes `commands` only. Verify: Edit an event page, add a node, Apply edited map; `.\build\tests\rat_tests.exe "[event],[edit]"`. Review: Approved.

## Bugs found

none.
