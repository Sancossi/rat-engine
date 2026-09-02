---
type: bug
area: Engine
status: Investigating
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

## Notes

Bake a `FenceSolid` on the ladder edge (`apply_max_step_up_skip = false`) spanning `y_lo`–`y_hi`. Do not require a separate authored fence. Update the MGS3 spec line that said overlap without climbing may pass the volume.
