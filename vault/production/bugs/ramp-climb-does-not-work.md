---
type: bug
area: Engine
status: Open
severity: Medium
sprint: Sprint 7
tags: [bug]
---

# Ramp climb does not work

Origin: chat 2026-09-01 (play). Tick path: [[feat: SimulationSession unified tick]]. Related (side entry, Fixed): [[ramp-allows-entry-from-side]]. Solids: [[feat: Bake height-grid and ramps to collision solids]].

## Repro

1. Play `grey_yard`.
2. Walk onto a ramp from its intended approach (not the side).

## Expected

The player walks up the ramp onto the higher cell (`max_step_up` / authored ramp slope).

## Actual

Ascent does not work (stuck, slide, or treated as a wall).

## Notes

Not a clone of [[ramp-allows-entry-from-side]] (that was side-entry lift). This is intended climb failing after the Sprint 7 tick/map split.

Follow-up from: [[feat: SimulationSession unified tick]]
