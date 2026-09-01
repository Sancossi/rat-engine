# Bug report: Stuck after falling from elevation or jumping into a ledge

**Status:** implemented (vault stays `Investigating` — not Fixed)  
**SHA:** `99651352b2f42fbd7eeebe652e9e8c211aaf98ac`  
**Tests:** `ctest --test-dir build --output-on-failure` — `100% tests passed, 0 tests failed out of 401`

## Root cause

Both play cases share one cause. Walk-off (and the jump apex) probes baked side faces at feet `>= y_hi`, so the cylinder is allowed to occupy the wall in XZ. The next grounded pose then snaps feet below `y_hi` while still overlapping. `integrate_player_surface` only reverts overlapping destinations, so a small step that remains inside the circle never escapes. The player is wedged against / inside the elevation.

Jump-into-face at low speed is the same overlap after the apex; a full-speed jump can still vault onto a 1.0 cell when feet clear the top (existing airborne-traversal tests).

Mini lip-skip in `cylinder_hits_walls` stays removed. Ramp west-climb / walk-off stay green: depenetration runs only when **grounded and not on a ramp**. Airborne depenetration was tried and pulled high-end walk-off back onto the trapezoid (`x≈1.60`).

## Evidence (RED)

- `[player][jump][surface]` “Drop from baked cube lands on lower ground and can walk” — landed at `x≈1.041`, `y=0`, then 30 east frames did not move (`1.041 > 1.541` failed).
- `[player][jump][surface]` “Jump into baked elevation face…” (speed 1.5 so the apex cannot fully enter the high cell) — after landing, could not walk west off the face.

Without `&map`, older walk-off tests still passed.

## Fix (minimal)

`depenetrate_cylinder_from_walls` pushes the XZ circle off the first hitting segment (same skip rules as `cylinder_hits_walls`: local span `<= max_step_up`, feet `>= y_hi`). Called from `integrate_player_frame_surface` after grounded blocker depenetration when `!on_ramp`. Landing from a cube drop / jump-wedge pops the body outside the face so the next walk step is free.

## Files

- `src/engine/include/rat/collision.hpp`
- `src/engine/src/collision.cpp`
- `src/engine/src/player_jump.cpp`
- `tests/player_jump_test.cpp`

Not touched: `player.cpp` wall probe / Mini lip, `cylinder_hits_walls` skip rules, vault/airborne traversal onto a 1.0 cell.

## Remaining concerns

- While still airborne and overlapping, XZ is still revert-on-hit; the pop-out happens on landing (grounded, off-ramp).
- Jumping onto a 1.0 cube when feet already clear `y_hi` remains allowed (`Airborne traversal can cross up if feet are high enough`).
- High-end ramp walk-off can still sit on the edge (`x==2`) as noted in the walk-off report.
- No extra grey_yard session test; unit maps cover the baked cube face.
