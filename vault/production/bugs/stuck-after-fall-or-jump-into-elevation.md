---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint:
tags: [bug]
---

# Stuck after falling from elevation or jumping into a ledge

Origin: chat 2026-09-01 (play). After [[cannot-fall-off-ramp]]. Solids: [[feat-bake-height-grid-and-ramps-to-collision-solids|feat: Bake height-grid and ramps to collision solids]].

## Repro

1. Play `grey_yard`.
2. Walk off a cube / raised cell so the player falls, **or** jump while pressed up against the face of an elevation.

## Expected

Falling lands on the lower ground and you can walk. Jumping into a wall stops horizontal motion and you drop; you are not trapped in the ledge.

## Actual

The player gets stuck (cannot move or stays wedged against / inside the elevation).

## Notes

Not a clone of [[cannot-fall-off-ramp]] (could not leave the ramp) or [[landing-on-jumpable-blocker-slides-player-off]] (slide off a blocker top). Suspect the walk-off probe `max(current feet, dest Y)` still hits the baked side face while airborne or after a drop.

Follow-up from: [[cannot-fall-off-ramp]]

## Resolution

Сход с уступа зондирует грань на высокой Y, потом ноги падают ниже `y_hi`, пока цилиндр ещё в стене; шаг только откатывает пересечение. После приземления вне рампы `depenetrate_cylinder_from_walls` выталкивает XZ. Verify: `.\build\tests\rat_tests.exe "*baked cube*"` / `"*Jump into baked*"` и `[player]`. Review: Approved.

## Bugs found

none.
