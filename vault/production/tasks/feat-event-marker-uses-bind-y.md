---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 11
due:
tags: [task]
---

# feat: Event marker uses bind Y

Intent: greybox event marker stays at y=0 even when `EventDef.y` binds a loft. Edit pick/gizmo should sit on the slab.

Acceptance: selected event with `y` set draws/picks at that height; unbound events stay on the grid sample.

Origin: [[feat-loft-event-height|feat: Loft event height]] (review Minor: `event_edit.cpp` marker). Взято в [[Sprint 11 — Set Move Route]].

## Resolution

`event_markers_from_map` и `EventRuntime::event_markers()` ставят маркер на `EventDef.y`, если поле задано; иначе — сэмпл сетки. Overlay-live dest по-прежнему сэмплит surface. Verify: Play/Edit — `loft_plank` cyan на плите (~y 2), не на земле. `.\build\tests\rat_tests.exe "*bind Y*"`. Review: Approved.

## Bugs found

none. Клик вьюпорта остаётся xz на плоскости y=0 (как у elevated height-grid маркеров) — не дефект этого среза.

