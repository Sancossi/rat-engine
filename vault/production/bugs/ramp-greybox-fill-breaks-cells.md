---
type: bug
area: Engine
status: Open
severity: Medium
sprint:
tags: [bug]
---

# Ramp greybox fill breaks cells and hides the high side

Origin: chat 2026-09-04 (play/edit). Greybox: [[feat: Edit and greybox edge walls]]. Terrain: `build_terrain_geometry` / `vs_debugdraw_lines` for fill tris (`greybox.cpp`). Related: gantry ramp `grey_yard` (8, 8) east 0→1, high cells y=1 at (9–11, 8).

## Repro

1. Open `grey_yard`, look at the east ramp at tile `(8, 8)` and the raised lane behind it.
2. Orbit / switch camera; compare ramp fill vs neighboring cubes.

## Expected

Ramp is a clean slope; the high-side cells (the “second floor” of that step) read as normal tiles with a grid.

## Actual

Fill/shader on the ramp looks crooked; the high side / raised cells don’t read (missing or warped), and the tile grid looks broken around the ramp.

Suspect: terrain fill uses `vs/fs_debugdraw_lines` (not a fill shader — see comment in `greybox.cpp`); sloped quads + grid lines on interpolated Y.
