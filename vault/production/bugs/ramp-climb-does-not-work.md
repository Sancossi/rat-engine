---
type: bug
area: Engine
status: Fixed
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

## Resolution

Bake клал бок рампы AABB высотой 1.0; западный заход упирался в стену. Endpoint Y + candidate Y до wall-теста. Глобальный lip-skip убран — Mini 0.45 снова бьётся при `y≈0.10`. Verify: `.\build\tests\rat_tests.exe "[collision]"` и west-climb в `[player]` / `[sim]`. Review: Approved after Mini-fence lip fix.

## Bugs found

none.

