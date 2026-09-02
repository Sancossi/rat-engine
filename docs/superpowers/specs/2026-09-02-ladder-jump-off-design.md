# Ladder climb: keep Y-axis, jump bounce-off

Vault: [[feat: Ladder toward-climb, jump grab, jump off]].
Depends: [[feat: Stacked surfaces caves and basements]] (shipped — overlap climb).
Related (not this card): [[feat: Step off ladder onto same-tile floor]].

Chat 2026-09-02: toward-face still climbs; jump grabs by reaching the volume; jump on a ladder bounces off the face.

Not this card: Interact-to-climb, map schema, editor tools, same-tile step-off at `y_hi`.

## Goal

Keep today’s climb axis. Add a **jump-off** that leaves the ladder instead of a grounded hop. Overlap still latches by itself.

## Play controls

| Situation | Input | Result |
| --- | --- | --- |
| Overlap, lockout = 0 | move **into** named face | +Y, clamped to `[y_lo, y_hi]` |
| Overlap, lockout = 0 | move **away** from face | −Y, same clamp |
| Overlap, lockout = 0 | tangent along the edge | XZ as today (still hits walls) |
| Overlap, lockout = 0 | `jump_pressed` | bounce-off (not `JumpTuning.jump_speed`) |
| No overlap, or lockout > 0 | move / jump | normal walk / jump / fall |
| Cylinder first overlaps a volume | (any) | latch: climb path, `vertical_speed = 0` |

Interact is unused. Jump is not required to mount. Falling or hopping **into** the volume latches without a second jump.

**Bounce vs first overlap:** bounce only if the cylinder already overlaps at the **start** of that jump substep (`jump_pressed` is still only substep 0). A hop that **enters** the volume later in the frame latches and does not bounce on that same `jump_pressed`. While latched, a consumed jump **buffer** uses bounce-off, not `jump_speed`.

## Bounce-off

On bounce (east example: face is +X, away is −X):

1. `grounded = false`.
2. `vertical_speed = ladder_hop_speed` (**4.0**, not `jump_speed` 7.5).
3. Instant XZ nudge **0.6** along face-away, then extra face-away XZ at **4.0** units/s while lockout lasts (added on top of WASD). Face-away: East −X, West +X, North +Z, South −Z (inverse of today’s climb `ix`/`iz` sign).
4. `ladder_lockout_left = 0.20` s.

Lockout ticks down every jump substep. While `> 0`, `overlapping_ladder` is ignored: no climb, no auto-latch, gravity and air move apply. After lockout, overlap latches again.

If the player holds into the face during lockout they may still be inside after 0.20 s and remount. That is intended. A tap-jump with no toward input must leave the volume.

## State and tick

- `JumpState::ladder_lockout_left` (seconds, default 0). `make_grounded_jump_state` zeros it.
- On bounce, store face-away unit XZ on `JumpState` (`ladder_bounce_x` / `ladder_bounce_z`); clear both when lockout hits 0. Replay mixes lockout and those two floats.
- `JumpTuning`: `ladder_lockout_seconds = 0.20`, `ladder_hop_speed = 4.0`, `ladder_bounce_speed = 4.0`. Instant nudge is a named constant **0.6** next to those (not map data).
- Climb stays in `integrate_player_surface`. The jump loop in `integrate_player_frame_surface` must **not** take the current “overlap → integrate_surface and `continue`” path when lockout > 0, and must run bounce **before** that swallow when `jump_pressed` and already overlapping.
- Debug snapshot read/write lockout and bounce XZ (missing keys → 0).
- No schema v4. Bake / `kLadderInset` / greybox unchanged.

## Tests (`[unit][player]`)

- Existing “East ladder climb reaches slab with move only” stays (into +Y, away −Y).
- East overlap + `jump_pressed`: `x` decreases, `y` increases, `ladder_lockout_left > 0`, next ticks do not clamp Y to the ladder while lockout lasts.
- After lockout, standing in the volume latches again.
- Ground jump that is **not** overlapping is unchanged (`jump_speed`).
- Tangent-vs-slab-side case unchanged.

## Out of scope

Same-tile step-off onto a slab at `y_hi` (away stays −Y). Parapet, Interact, new editor UX, coyote-only grab, ignoring ladders until landing.
