# Sprint 3 Elevation Acceptance

## Preconditions
- Build `rat-editor` and `rat_tests`.
- Start editor in Play mode with map `grey_yard`.

## Play Verification
- Confirm legacy quest flow still works: intro autorun, foreman interaction, scrap pickup, turn-in.
- Walk to the isolated elevation lane near `(x=7.2, z=8.5)`.
- Move forward onto the ramp tile around `(8, 8)` and verify smooth grounded rise from `y0` to platform `y1`.
- On platform, face the low blocker near `x=10.2..10.8`, press `Space` to jump, and clear it with default tuning.
- Land on the blocker top once: confirm small X/Z movement stays at `top_y`,
  jumping works, and walking beyond the footprint starts a smooth coyote fall.
- Approach the ramp midpoint from tiles `(8, 7)` and `(8, 9)` and confirm side
  entry is blocked. Confirm `(7, 8)` and low-side diagonals `(7, 7)` /
  `(7, 9)` remain valid approaches.
- Land beyond blocker and move to elevated action event near tile `(11, 8)`.
- Press `E` and verify event executes (text + switch update).
- Move player at lower-level `y` near the same `x/z` and verify prompt does not appear and trigger is rejected.

## Edit Verification
- Switch to Edit mode and set tile/ramp coordinates in elevation panel.
- Use `+ Step` and `- Step`; after each successful click, confirm `Set Y` immediately matches current map tile value (Task7 fix).
- Edit ramp low/high values, apply, and verify marker/terrain update.
- Save map, hot-reload via `F5`, and confirm the same elevation lane remains intact.

## Camera + Controls Sweep
- Repeat interaction checks in `TopDown`, `Tilt45`, and `ThreeQuarter`.
- Verify movement and jump readability from each camera.
- Re-check `Space` jump and `E` interact around blocker/event in at least two modes.
