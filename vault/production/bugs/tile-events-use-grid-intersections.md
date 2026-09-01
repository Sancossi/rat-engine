---
type: bug
area: Engine
status: Fixed
severity: High
tags: [bug]
notion_id: 3cdf3827-36cc-8164-baf7-c3bfa8173e57
---

# Tile events use grid intersections instead of cell centers

## Repro

Action prompt appears while player is still visibly outside/diagonal from event marker; screenshot 2026-08-31.

Expected: tile event marker and AABB centered at `(x+0.5, z+0.5)*tile_size`, matching player snap convention.

Actual: both marker and trigger use `(x,z)*tile_size`.

Fix source conversion and update tests/sample coordinates.

## Follow-up 2026-08-31

После переноса в центры Action всё ещё срабатывает рано: AABB игрока пересекается с AABB целой клетки события почти за одну клетку и по диагонали.

Fix: Action использует радиус до центра marker; PlayerTouch/EventTouch сохраняют AABB-семантику.

## Fixed

Commit `08f7538`: tile Action events now activate within 0.65 tile from marker center. Touch triggers and explicit event volumes retain AABB overlap. Regression suite: 48/48 passed.

## Follow-up: coordinate convention

Player at world `(3, -2)` is rendered on grid-line intersection, while tile entities are expected at cell centers. Audit player spawn, grid rendering and tile-to-world conversion; use one convention: integer tile coordinate → world center `(tile + 0.5) * tile_size`.

## Root cause confirmed: crate_notice volume

Player coordinates are continuous world coordinates and may lie on grid lines. The actual early trigger was `crate_notice`: its volume extended 0.5 tile beyond the crate blocker on every side. Volume changed from `(2.5,-1.5)-(5.5,1.5)` to blocker bounds `(3,-1)-(5,1)`, so PlayerTouch now begins when the player's body reaches the blocker. Added map regression assertions; 48/48 tests pass. Awaiting visual confirmation.

User visually confirmed the corrected trigger distance on 2026-08-31.
