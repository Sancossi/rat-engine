# MGS3 ladder climb

Vault: [[feat: MGS3 ladder climb]].
Depends: [[feat: Camera-aligned walk]] (shipped).
Absorbs: [[feat: Step off ladder onto same-tile floor]].
Replaces feel of: [[feat: Ladder toward-climb, jump grab, jump off]] (bounce / auto-latch / camera-projected tangent).

Chat 2026-09-02: camera does not turn on climb; do the full MGS3 ladder, not another WASD projection tweak.

## Goal

Climb is a **rail mode**. Interact mounts (walk-by does not). Camera sits behind the player looking at the rungs so screen-forward is up. Only ±Y on the rail. Jump does nothing. Top + forward = step onto the landing; bottom + back = step off.

## Mount

`PlayerFrameInput.interact_pressed` (session: consume interact for player first; events get it only if still not climbing).

Mount when all of:

1. Not already climbing.
2. `interact_pressed`.
3. In a **mount zone** of a baked `LadderVolume`: cylinder overlaps the volume, **or** feet are within `0.45` of `y_lo` and the footprint is on the approach side (face-away of the volume), **or** grounded with feet within `0.45` of `y_hi` and XZ overlapping the owner tile / volume.
4. Look / move is not required (Action in front or above, like the MGS3 manual).

On mount: snap XZ to the volume center, clamp Y to `[y_lo, y_hi]`, `climbing = true`, `grounded = true`, `vertical_speed = 0`, store `climb_into_x/z` (East +1,0; West −1,0; North 0,−1; South 0,+1). Do **not** auto-latch on overlap without interact.

## Climb tick

If `climbing` (from previous substep / mount this substep):

- Snap XZ to rail center every substep.
- Ignore A/D (tangent = 0). Climb axis = dot(move, into). Empty `climb_move` → use `move`.
- `dot > 0` → +Y; `dot < 0` → −Y; speed = `player.speed`. Clamp to `[y_lo, y_hi]`.
- `jump_pressed` / jump buffer: **ignore** (no bounce, no hop, no lockout).
- Gravity off while climbing.

`integrate_player_surface` **does not climb**. Ground/air motors pass `ignore_ladders = true`. Climb lives only in `tick_climb_substep`.

## Dismount

- **Top:** Y ≥ `y_hi - 0.05` and climb axis **up** (`dot > 0`): if `query_solid_support` at a face-away nudge (`0.35` along −into) finds a standable top ≥ `y_hi - 0.05`, place feet on that support, `climbing = false`, grounded walk. If no support, clamp at `y_hi` and stay climbing.
- **Bottom:** Y ≤ `y_lo + 0.05` and climb axis **down**: place on approach (nudge face-away `0.35`), `climbing = false`. If there is ground/support, stand; else fall.
- Overlap without climbing is walk/air (can pass the volume).

Leave bounce fields in `JumpState` at 0. Stop applying bounce/lockout in the climb path. Do not delete snapshot keys.

## Camera

Pure `rat_core` (no bgfx):

```
struct ClimbCameraPose { Vec3 eye; Vec3 focus; };
ClimbCameraPose climb_camera_pose(Vec3 player, float into_x, float into_z);
```

`kClimbCameraBack = 10`, `kClimbCameraHeight = 8`. Eye = player − into * back + (0, height, 0). Focus = player. Degenerate into → treat as East.

`camera_relative_move` from that pose + screen W (0, +1) has **positive dot with into** (W = up the ladder).

Greybox: while `JumpState.climbing`, `rebuild_camera` uses this pose and look-at with up +Y (including TopDown). `C` does not change the climb shot. On dismount, restore the selected `CameraMode`.

Apply the lock **before** `map_input_frame` when already climbing so this frame’s WASD matches the shot.

## Session interact

Player integrate sees interact. If the tick ends `climbing`, EventRuntime gets `interact_pressed = false` for that tick (mount consumed Action). Dialog-open still acknowledges first (existing order).

## Tests

`[unit][camera]`: East pose eye.x < player.x; W via `camera_relative_move` has axis_x > 0.5.

`[unit][player]`: overlap + move only does **not** set `climbing`; overlap + interact does; jump while climbing stays on rail (`lockout == 0`); East interact then into-face move raises Y; top + up lands on same-tile slab; bottom + down leaves climbing; tangent A/D does not change XZ.

Update or drop bounce/lockout/auto-latch tests that contradict this spec.

## Out of scope

Schema v4, editor ladder UX, tree/ivy, hanging/grip gauge, auto-climb cutscenes.
