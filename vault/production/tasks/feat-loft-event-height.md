---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 10
due:
tags: [task]
---

# feat: Loft event height

Intent: tile/touch events bind to height-grid/ramp Y, not slab tops. A 2nd-floor trigger on the same XZ as a loft needs an explicit bind (`y` or “top solid”). Not in Sprint 9 DoD.

Acceptance: author an event on a slab cell that fires only when the player stands on that slab; walking the ground under it does not fire. Schema + Play + a `grey_yard` fixture.

Origin: [[events-match-ground-while-on-slab]] (review: tile events cannot sit on loft without a field). Взято в [[Sprint 10 — Grey yard content]].
