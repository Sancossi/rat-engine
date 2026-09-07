---
type: bug
area: Engine
status: Fixed
severity: High
sprint: Sprint 8
tags: [bug]
---

# Walk off slab teleports to the ground

Origin: chat 2026-09-02 playtest. `grey_yard` loft slabs `top_y` 2.0; hole at `(2, 5)`. Related: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]] (acceptance: дыра ведёт вниз). Взято в [[Sprint 8 — Play feel and authoring]].

Playtest 2026-09-02 evening (after keep-Y in `follow_standing_sample` + full Release rebuild): **still snaps**. Reopened.

Follow-up: [[walk-through-ladder]]

Follow-up from: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]]

## Repro

1. Play `grey_yard`, climb the East ladder to the second floor (slab ~Y 2).
2. Walk off the loft edge or into the hole (`(2, 5)`), or slide off the slab. Do **not** jump.

## Expected

Leave support → airborne, gravity, **smooth fall** onto the ground (Y 0). Same curve as **jumping** off the same ledge (Fall / `faster_fall_gravity`), not a one-frame Y teleport.

## Actual

Player **teleports** to the first floor instead of falling.

## Resolution

Walk-off already left the loft at Y≈2; the teleport was mid-fall: the cylinder still overlapped a **neighbor** slab after the feet point left its tile, so `clamp_to_ceiling` slammed Y to `y_lo − height`. Ceiling now uses the same point-in-tile rule as standing support. Verify: Play `grey_yard` hole `(2, 5)` or south loft edge; `.\build\tests\rat_tests.exe "*loft*"`. Review: Approved.

## Bugs found

none.
