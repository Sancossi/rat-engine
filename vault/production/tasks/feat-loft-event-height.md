---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 10
due:
tags: [task]
---

# feat: Loft event height

Intent: tile/touch events bind to height-grid/ramp Y, not slab tops. A 2nd-floor trigger on the same XZ as a loft needs an explicit bind (`y` or “top solid”). Not in Sprint 9 DoD.

Acceptance: author an event on a slab cell that fires only when the player stands on that slab; walking the ground under it does not fire. Schema + Play + a `grey_yard` fixture.

Origin: [[events-match-ground-while-on-slab]] (review: tile events cannot sit on loft without a field). Взято в [[Sprint 10 — Grey yard content]].

Follow-up: [[feat: Event marker uses bind Y]]

## Resolution

Optional `EventDef.y` (`"y": 2.0`). Проба solid на authored Y; без поля — земля как в Sprint 9. `grey_yard`: `loft_plank` на (0,4), switch 43. Inspector: Bind Y / Event Y / On slab. Verify: Play — залезть на лофт, E на (0,4); с земли под клеткой не fire. `.\build\tests\rat_tests.exe "[event]"`. Review: Approved.

## Bugs found

none.
