---
type: bug
area: Engine
status: Investigating
severity: High
sprint: Sprint 8
tags: [bug]
---

# Walk off slab teleports to the ground

Origin: chat 2026-09-02 playtest. `grey_yard` loft slabs `top_y` 2.0; hole at `(2, 5)`. Related: [[feat: Stacked surfaces caves and basements]] (acceptance: дыра ведёт вниз). Взято в [[Sprint 8 — Play feel and authoring]].

## Repro

1. Play `grey_yard`, climb to the second floor (slab ~Y 2).
2. Walk off the loft edge or into the hole (`(2, 5)`), or slide off the slab.

## Expected

Leave support → airborne, gravity, **smooth fall** onto the ground (Y 0). No Y snap.

## Actual

Player **teleports** to the first floor instead of falling.

Suspect walk-off / `query_solid_support` snaps feet to ground_y under the same XZ instead of Fall. Not a clone of [[stuck-after-fall-or-jump-into-elevation]] (wedge) or [[cannot-fall-off-ramp]].

Follow-up from: [[feat: Stacked surfaces caves and basements]]
