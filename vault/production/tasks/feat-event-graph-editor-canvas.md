---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Event graph editor canvas

Intent: в Edit у выбранного event/page — холст нод (trigger, conditions, commands, branch). Правка графа пишет документ и hot-apply как сейчас список команд.

Acceptance: place/connect/delete nodes; compile on apply; inspector list остаётся fallback. MVP без полного MZ-набора. Не Ruby/JS VM.

Origin: [[feat: Event node graph authoring]]. Depends: [[feat: Event graph model and compile]]. Related: [[s2-event-inspector-pages-stub]]. Взято в [[Sprint 8 — Play feel and authoring]].

Compile review leftovers (do in this slice if cheap): on apply/save write `commands` from `compile_event_graph`; add a join-after-branch golden (then/else reconverge then continue); treat sequence edges out of `conditional_branch` as compile errors.
