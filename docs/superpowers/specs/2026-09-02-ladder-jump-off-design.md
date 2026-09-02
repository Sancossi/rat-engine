# Ladder climb: camera up/down, jump bounce-off

Vault: [[feat: Ladder toward-climb, jump grab, jump off]].
Depends: [[feat: Stacked surfaces caves and basements]] (shipped — overlap climb).
Related (not this card): [[feat: Step off ladder onto same-tile floor]].

Chat 2026-09-02: jump grabs by reaching the volume; jump on a ladder bounces off the face.
Clarification: **up vs down follows the camera**, not world WASD. Walk off-ladder stays world-aligned (W=−Z, D=−X).

Not this card: Interact-to-climb, map schema, editor tools, same-tile step-off at `y_hi`.

## Goal

Keep overlap auto-latch. Climb **up** is the screen direction toward the ladder; opposite is down. Jump on a ladder bounce-off the face with lockout.

## Play controls

Walking (not overlapping, or lockout > 0): unchanged `world_aligned_move` (camera switch does not rotate WASD).

On a ladder (lockout = 0): steer with **`camera_relative_move`** from the same WASD + current camera eye/focus. Split that world XZ against the ladder face:

- Component **into** the named face → +Y
- Opposite → −Y
- Remainder → tangent XZ (still hits walls)

Examples: ladder in the look direction → W up, S down. Ladder to the right of the view → D up, A down.

| Situation | Input | Result |
| --- | --- | --- |
| Overlap, lockout = 0 | camera-steer into face | +Y, clamped to `[y_lo, y_hi]` |
| Overlap, lockout = 0 | camera-steer away | −Y, same clamp |
| Overlap, lockout = 0 | `jump_pressed` | bounce-off (not `JumpTuning.jump_speed`) |
| No overlap, or lockout > 0 | move / jump | normal walk / jump / fall |
| Cylinder first overlaps a volume | (any) | latch: climb path, `vertical_speed = 0` |

Tests that pass only world `MoveInput` (no camera steer) keep today’s into-face mapping (climb_move empty → use walk axes).

Interact is unused. Jump is not required to mount.

**Bounce vs first overlap:** bounce only if already overlapping at the **start** of that jump substep. A hop that **enters** later latches and does not bounce on that same `jump_pressed`. Leftover jump **buffer** bounces only while **already latched** (`was_climbing`); a first overlap with a live air-jump buffer latches and clears the buffer. While latched, a consumed jump buffer uses bounce-off, not `jump_speed`.

## Bounce-off

On bounce (east example: face is +X, away is −X):

1. `grounded = false`.
2. `vertical_speed = ladder_hop_speed` (**4.0**, not `jump_speed` 7.5).
3. Instant XZ nudge **0.6** along face-away, then extra face-away XZ at **4.0** units/s while lockout lasts (added on top of WASD). Face-away: East −X, West +X, North +Z, South −Z (inverse of today’s climb `ix`/`iz` sign).
4. `ladder_lockout_left = 0.20` s.

Lockout ticks down every jump substep. While `> 0`, `overlapping_ladder` is ignored: no climb, no auto-latch, gravity and air move apply. After lockout, overlap latches again.

If the player holds into the face during lockout they may still be inside after 0.20 s and remount. That is intended. A tap-jump with no toward input must leave the volume.

## State and tick

Each jump substep **resolves a locomotion motor** then runs that state’s integrate:

- **Climb** — overlap and lockout = 0: camera-steer climb in `integrate_player_surface`; `JumpState.climbing = true`. Jump/buffer → bounce and **Fall**, not a grounded hop.
- **Idle / Walk** — grounded, not Climb: world-aligned walk (existing surface/blocker path).
- **Jump / Fall** — airborne, including ladder lockout: gravity, coyote, bounce XZ while lockout > 0.

`locomotion_from` returns Climb first if `climbing`, else Idle/Walk/Jump/Fall as today. Animation names add `"Climb"`.

- `JumpState::ladder_lockout_left` (seconds, default 0). `make_grounded_jump_state` zeros lockout, bounce, and `climbing`.
- On bounce, store face-away unit XZ (`ladder_bounce_x` / `ladder_bounce_z`); clear both when lockout hits 0. Replay mixes lockout, bounce XZ, and `climbing`.
- `JumpTuning`: `ladder_lockout_seconds = 0.20`, `ladder_hop_speed = 4.0`, `ladder_bounce_speed = 4.0`. Instant nudge is `kLadderBounceNudge` **0.6**.
- Play fills `InputFrame.climb_move` via `camera_relative_move`. Headless tests may leave `climb_move` zero and use walk axes.
- Debug snapshot read/write lockout, bounce XZ, `climbing` (missing keys → 0 / false).
- No schema v4. Bake / `kLadderInset` / greybox unchanged.

## Tests (`[unit][player]`)

- Existing “East ladder climb reaches slab with move only” stays (into +Y, away −Y).
- East overlap + `jump_pressed`: `x` decreases, `y` increases, `ladder_lockout_left > 0`, next ticks do not clamp Y to the ladder while lockout lasts.
- After lockout, standing in the volume latches again.
- Airborne with a live jump buffer, then entering the volume: latch (lockout 0, climbing), not bounce.
- Ground jump that is **not** overlapping is unchanged (`jump_speed`).
- Tangent-vs-slab-side case unchanged.

## Out of scope

Same-tile step-off onto a slab at `y_hi` (away stays −Y). Parapet, Interact, new editor UX, coyote-only grab, ignoring ladders until landing.
