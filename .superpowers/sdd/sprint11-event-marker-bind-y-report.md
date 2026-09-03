# Sprint 11 / feat: Event marker uses bind Y

**Status:** Implemented (vault left `In progress`; implementer does not set Done / In review)

**Branch:** `feat/sprint-4`

**Commit message:** `Sit event markers on EventDef bind Y.`

**Test tag:** `[unit][hot_apply]` (authored Edit markers) and `[unit][events]` (Play spawn markers). Filter used: `*bind Y*`.

## Functions that honor `EventDef.y`

- `rat::event_markers_from_map` (`hot_apply.cpp`) — tile and volume markers. Bind Y when set; else `SurfaceQuery::sample` at xz.
- `EventRuntime::event_markers()` (`event_runtime.cpp`) — spawn/unmoved events with `EventDef.y` sit on bind Y. Overlay-live (Set Move Route dest) still samples dest surface. Unbound stays on the grid sample.

Edit pick/gizmo uses those marker positions (`GreyboxScene` draw + `event_markers_from_map` / Play `event_markers()`). Viewport xz pick unchanged.

Did not rewrite Set Move Route opcode/collision. Did not edit `data/maps/grey_yard.json` (`loft_plank` already has `"y": 2.0`).

## TDD

### RED

`Event markers sit on EventDef bind Y not ground sample` and `Event runtime spawn markers use EventDef bind Y`:

```
REQUIRE( markers[0].y == Approx(2.0f) )
with expansion:
  0.0f == Approx( 2.0 )
```

SurfaceQuery at xz returned ground 0 under the slab.

### GREEN

Bound tile `y: 2.0` → marker y=2. Unbound neighbor → y=0. Bound volume `y: 1.5` → marker y=1.5.

## Tests

```
.\build\tests\rat_tests.exe "*bind Y*"
  All tests passed (20 assertions in 2 test cases)

.\build\tests\rat_tests.exe "[unit][hot_apply],[unit][events],[unit][event_edit],[unit][route]"
  All tests passed (365 assertions in 55 test cases)
```

VsDevCmd x64 + `cmake --build build --config Release --target rat_tests`. `rat_core` glfw-free.
