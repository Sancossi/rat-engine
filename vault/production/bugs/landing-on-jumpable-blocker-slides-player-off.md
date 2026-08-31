---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 3
tags: [bug]
---

# Landing on jumpable blocker slides player off

Origin: [[s3-acceptance-stairs-jump-and-elevated-event]]

## Repro

1. In `grey_yard`, jump onto the low jumpable blocker on the elevated lane.
2. Land while the player footprint still overlaps the blocker top.

## Expected

The player lands on `top_y`, can stand, move, jump, and walk off normally.

## Actual

Descent handling ejects the player beyond the blocker face, so the player
slides off instead of using the top as a support.

## Resolution

`JumpState` now tracks a validated blocker support. Descending swept contact
lands on `top_y`; idle, movement, jumping, and coyote walk-off preserve world
height. Edge landings, thin blockers, stale supports, and raised terrain have
regression coverage.

## Acceptance follow-up

On the narrow `grey_yard` blocker, even a small movement after landing can
release support and drop the player immediately. Support must remain until the
player actually walks beyond its footprint, then preserve world Y and fall.

Fixed by removing narrow-blocker forced ejection. Support now follows actual
player AABB overlap; small X/Z movement remains on `top_y`, and leaving starts
coyote fall without a position snap.
