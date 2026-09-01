---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 7
tags: [bug]
---

# Cannot fall off ramp

Origin: chat 2026-09-01 (play). After [[ramp-climb-does-not-work]]. Solids: [[feat: Bake height-grid and ramps to collision solids]].

## Repro

1. Play `grey_yard`.
2. Walk onto a ramp from its intended approach.
3. Keep walking off the high or low end (or the open side) as if leaving the slope into air or a lower cell.

## Expected

Leaving the ramp footprint drops the player (gravity / step-down), same as walking off a cube top.

## Actual

The player stays stuck to the ramp and cannot fall off.

## Notes

Not a clone of [[ramp-climb-does-not-work]] (ascent) or [[ramp-allows-entry-from-side]] (side-entry lift). Suspect the climb fix (candidate Y before wall-test / trapezoid sample) still treating ramp Y as support outside the walk-off.

Follow-up from: [[ramp-climb-does-not-work]]

## Resolution

Climb-фикс зондировал стены на Y клетки назначения; сход с рампы брал нижнего соседа и цеплялся за трапецию. `integrate_player_surface` теперь пробивает стены на `max(текущие ноги, dest Y)`. Verify: `.\build\tests\rat_tests.exe "*walk-off*"` и `[player]`. Review: Approved.

Follow-up: [[stuck-after-fall-or-jump-into-elevation]] — play 2026-09-01, после фикса схода с рампы застревание при падении с возвышения или прыжке в упор.

## Bugs found

- [[stuck-after-fall-or-jump-into-elevation]] — play 2026-09-01, застревание после падения с возвышения или прыжка в упор.
