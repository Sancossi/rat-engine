---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 11
due:
tags: [task]
---

# feat: Event marker uses bind Y

Intent: greybox event marker stays at y=0 even when `EventDef.y` binds a loft. Edit pick/gizmo should sit on the slab.

Acceptance: selected event with `y` set draws/picks at that height; unbound events stay on the grid sample.

Origin: [[feat: Loft event height]] (review Minor: `event_edit.cpp` marker). Взято в [[Sprint 11 — Set Move Route]].
