---
type: bug
area: Engine
status: Fixed
severity: High
sprint: Sprint 8
tags: [bug]
---

# Walk through ladder

Origin: chat playtest 2026-09-02 evening (same pass as [[walk-off-slab-teleports-to-ground]]). Ladder bake is overlap-only (`LadderVolume`); MGS3 spec allowed walking through the volume without climbing. Playtest: that feels wrong — the face should be a **wall**.

Взято в [[Sprint 8 — Play feel and authoring]]. Related: [[feat: MGS3 ladder climb]], [[edge-walls-passable-from-adjacent-side]].

## Repro

1. Play `grey_yard`. East ladder on tile `(2, 4)`, `y` 0–2.
2. On the ground, walk through the ladder face (e.g. from `(3, 5)` / `(3, 4)` west into the owner tile, or from under the loft east out through the rungs). Do not press Interact.

## Expected

The ladder face blocks the cylinder like a fence/wall for `[y_lo, y_hi]`. You cannot walk through the rungs. Interact still mounts from the approach side; climb rail unchanged.

## Actual

The volume is overlap-only. Walking through the face is free.

## Resolution

`append_ladders` bakes a `FenceSolid` on the face (`apply_max_step_up_skip = false`, y span = ladder). Ground walk cannot cross the rungs. Interact still mounts when pressed against the wall (approach `y_lo` zone). Climb volume/rail unchanged. Verify: Play `grey_yard` walk west into East ladder `(2, 4)` without E; then E to climb. `.\build\tests\rat_tests.exe "[collision],[unit][player]"`. Review: Approved.

## Bugs found

none.
