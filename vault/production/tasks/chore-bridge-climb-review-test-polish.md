---
type: task
area: Engine
status: Done
review: Approved
task_type: Chore
sprint: Sprint 15
due:
tags: [task]
---

# chore: Bridge climb review test polish

Intent: Minor из ревью [[cannot-climb-onto-grey-yard-bridge|Cannot climb onto grey_yard bridge]] (Approved): в `player_test` `REQUIRE_FALSE(stood_on_occupancy_solid)` всегда true — в фикстуре нет `kind: Solid`. Проверить, что игрок стоит на `floor_slabs`, либо убрать пустой assert.

Acceptance: тест ломается, если пролёт заменён occupancy solid.

Origin: [[cannot-climb-onto-grey-yard-bridge|Cannot climb onto grey_yard bridge]]

## Resolution

Implemented in 2a44876; independent review Approved. Full verification 698/698 C++ tests passed, focused geometry/bridge run 6 tests / 1,314 assertions. See [[s15-geometry]] for shared evidence.

## Bugs found

Slab-height mismatch and review arithmetic findings resolved; see [[s15-geometry]].

Scheduling: carried from Sprint 14 into [[Sprint 15 — Stabilization]]; original Origin retained.
