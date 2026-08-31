---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 6
tags: [bug]
---

# Edge walls hang in the air

Origin: [[Sprint 6 — Viewport map edit]] (chat). Greybox: [[feat: Edit and greybox edge walls]].

## Repro

1. Open `grey_yard` (or Place cube, then Mini/Full fence on that tile or a neighbour).
2. Orbit camera (C) and look at the fence / cube sides from below or from the low neighbour.

## Expected

Fence sits on the tile surface along that edge (bottom of the quad meets the ground/top of the cube). Raised height-grid cells have vertical sides down to the neighbour, including outer map-border edges.

## Actual

Walls on edges look suspended — a gap between the quad and the terrain.

## Notes

Likely `build_edge_barrier_faces` uses a single `owner_top` from the tile center for both edge ends (`y0_lo`/`y1_lo`), so the fence does not follow the two edge corners. Height-grid **outer** shared edges also skip `build_terrain_side_faces` (only east/south *internal* neighbours), so a cube on the grid border has a floating top.

## Resolution

`build_edge_barrier_faces` ставит низ забора на два угла owner-edge, верх = угол + `edge.height`. `build_terrain_side_faces` рисует внешние границы height-grid против implied height 0, если клетка выше 0. Проверка: `[terrain]` в `rat_tests`; greybox `grey_yard` — забор и бок куба на краю сетки без щели.

Bugs found: none.
