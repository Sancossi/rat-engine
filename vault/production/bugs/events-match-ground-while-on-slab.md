---
type: bug
area: Engine
status: Investigating
severity: Medium
sprint: Sprint 9
tags: [bug]
---

# Events match ground height while the player stands on a slab

## Repro

1. Place a floor slab `top_y=2` on a ground cell that also has a tile/touch event.
2. Play: climb or stand on the slab (player.y ≈ 2).
3. Walk the same XZ as the event.

## Expected

The ground-level event does not fire: player height does not match the event surface.

## Actual

`EventRuntime::event_height_matches_player` still compares `SurfaceQuery.sample` (ground/ramp), not `player.y` / baked solids. Standing on the slab still matches the ground event.

Origin: [[feat: Stacked surfaces caves and basements]] (final slice review). Взято в [[Sprint 9 — First playable loop]].

Review: pending after `afc22e7` (`event_height_matches_player` uses solid support / `player.y`, not height-grid-only).
