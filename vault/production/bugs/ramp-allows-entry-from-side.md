---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 3
tags: [bug]
---

# Ramp allows entry from side

Origin: [[s3-acceptance-stairs-jump-and-elevated-event]]

## Repro

1. Approach the east-facing ramp at tile `(8, 8)` from `(8, 7)` or `(8, 9)`.
2. Move directly onto the ramp through its north or south edge.

## Expected

Side edges block entry. The low west edge accepts approaches from `(7, 8)` and
diagonal low-side neighbours `(7, 7)` / `(7, 9)`; the high edge connects to its
matching elevated terrain.

## Actual

Entering through a side edge samples the ramp and immediately lifts the player
onto it.

## Resolution

Positive height changes bypass `max_step_up` only while moving within the same
ramp. Entering from flat terrain now obeys the step cap: midpoint side entry is
blocked, while the low edge, its low-side diagonals, and matching high platform
remain connected.
