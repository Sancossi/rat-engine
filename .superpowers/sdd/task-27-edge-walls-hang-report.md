# Task 27 report: Edge walls hang in the air

## What you implemented

Greybox edge fences and height-grid cube sides meet the terrain along the edge.

- `build_edge_barrier_faces`: bottoms use the two owner-tile corner Ys of that edge (`y_ne`/`y_se` for east, etc.), tops = corner + `edge.height`. No single tile-center `owner_top`.
- `build_terrain_side_faces`: still pairs only east/south *internal* neighbours (no duplicate shared walls). On outer height-grid borders, emits a vertical face vs implied outside height **0** when the border tile is above 0 (same min/max Y as internal 0-vs-1 walls).

Collision still samples owner top at tile center. Vault card stays **Investigating**. Do not set Fixed / In review.

## What you tested and results

- Focused: `.\build\tests\rat_tests.exe "[terrain]"` → **119 assertions in 25 test cases, all passed**
- Full (before commit): `.\build\tests\rat_tests.exe` → **2184 assertions in 278 test cases, all passed**

Existing fence tests stayed green. Internal 0-vs-1 / ramp-trapezoid / origin cases still assert the shared-edge wall; they no longer require `faces.size() == 1` because a raised border tile also has outer sides.

## TDD Evidence

**RED** (center `owner_top`; outer borders not emitted):

```
Edge barrier faces use both owner-edge corners when those Y differ
  REQUIRE( face.y0_lo == Catch::Approx(1.0f) )
  4.0f == Approx( 1.0 )

Terrain side faces emit an outer east wall down to implied height 0
  REQUIRE( found_east )
  false

test cases:  25 |  23 passed | 2 failed
assertions: 127 | 125 passed | 2 failed
```

(Flat raised-tile fence case passed on the old center sample because all four corners equal `ground_y`.)

**GREEN** (focused):

```
.\build\tests\rat_tests.exe "[terrain]"
All tests passed (119 assertions in 25 test cases)
```

**GREEN** (full suite before commit):

```
.\build\tests\rat_tests.exe
All tests passed (2184 assertions in 278 test cases)
```

## Files changed

- `src/engine/src/terrain_geometry.cpp`
- `tests/terrain_geometry_test.cpp`
- `.superpowers/sdd/task-27-edge-walls-hang-report.md`

## Follow-ups / bugs found

Bugs found: none.

Did not change jump/collision Y, schema v3, mouse terrain, or undo.
