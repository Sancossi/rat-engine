# Camera-aligned walk (Play)

Vault: [[feat: Camera-aligned walk]].
Prerequisite for: [[feat: MGS3 ladder climb]].
Supersedes Play behavior of [[camera-switch-changes-wasd-world-directions]] (Edit stays without player WASD).

Chat 2026-09-02: world-aligned walk plus camera-projected climb feels wrong; next slice is Play walk that follows the camera.

## Goal

In Play, screen WASD maps to world XZ through `camera_relative_move` (already used for `climb_move`). W is “into the shot.” Switching camera with `C` **does** rotate W — that is intended.

## Mapping

`map_input_frame` already runs only when `gating.player_control` (Play). Change:

```
frame.move = camera_relative_move(screen_x, screen_z, camera_eye, camera_focus);
frame.climb_move = camera_relative_move(...);  // same; keep the field
```

`camera_relative_move` is unchanged:

- Look XZ = `focus - eye` flattened. Forward = that unit vector. Right = 90° clockwise from +Y.
- Degenerate look (top-down, `flen <= 1e-6`): `(-screen_x, -screen_z)` — **same numbers as** `world_aligned_move`. Headless tests that omit eye/focus stay stable.
- `world_aligned_move` stays in the API for tests and any caller that wants fixed W=−Z, D=−X.

Editor does not feed player WASD (`player_control` false). Do not re-open the Edit-side of the old camera-switch bug.

## Tests (`[unit][input]`)

- Default eye/focus `{}`: `frame.move` equals `world_aligned_move` (degenerate fallback).
- Eye `{0,8,0}` focus `{4,0,0}` (look +X), W: `move.axis_x > 0.5`, matches `camera_relative_move`.
- Same fixture: `climb_move == move`.
- Gating (keyboard captured / player_input_blocked) still zeros `move`.

Physics integrate tests that pass `MoveInput` directly are unchanged.

## Out of scope

MGS3 camera lock, Interact-to-mount, 1D climb rail, bounce removal, same-tile step-off. Those stay on [[feat: MGS3 ladder climb]].
