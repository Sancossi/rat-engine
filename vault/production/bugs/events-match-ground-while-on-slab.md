---
type: bug
area: Engine
status: Fixed
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

Follow-up: [[feat: Loft event height]]

## Resolution

`EventRuntime` печёт `CollisionWorld` на load. Стоячая высота — `query_solid_support` у ног; Y события — solid в XZ тайла с пробы на height-grid/ramp, поэтому плита сверху не перетягивает ground-event. Verify: Play — встать на плиту над тайловым событием (не fire), пройти землю под той же клеткой (fire); `.\build\tests\rat_tests.exe "[event]"`. Review: Approved.

## Bugs found

none.
