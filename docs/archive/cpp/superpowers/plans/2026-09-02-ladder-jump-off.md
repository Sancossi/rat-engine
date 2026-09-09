# Ladder Jump-Off Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep into-face +Y / away −Y climb; overlap still latches; jump on a ladder bounce-off the face with a short lockout so the cylinder does not remount immediately.

**Architecture:** `JumpState` holds lockout + face-away bounce XZ. `integrate_player_frame_surface` bounces before the overlap-climb `continue`, ignores ladders while lockout > 0, and applies bounce XZ on top of WASD. Climb stays in `integrate_player_surface` when lockout is 0.

**Tech Stack:** C++20, Catch2, `rat_core`. Windows: VsDevCmd x64 + existing `build/` Ninja Release.

## Global Constraints

- Spec: `docs/archive/cpp/superpowers/specs/2026-09-02-ladder-jump-off-design.md`
- Vault: `vault/production/tasks/feat-ladder-toward-climb-jump-grab.md` — implementer does **not** set Done / In review
- Climb axis unchanged: into named face +Y, away −Y, tangent XZ still hits walls
- Overlap auto-latches when `ladder_lockout_left <= 0`
- Bounce (already overlapping at **start** of substep): `vertical_speed = 4.0` (not `jump_speed` 7.5), instant nudge **0.6** face-away, extra face-away XZ **4.0** u/s during lockout, `ladder_lockout_left = 0.20`
- Face-away: East −X, West +X, North +Z, South −Z
- Jump buffer while latched uses bounce-off, not `jump_speed`
- Bounce only if overlapping at start of substep; a hop that **enters** later latches and does not bounce on that same `jump_pressed`
- `mix_jump` + debug snapshot include `ladder_lockout_left`, `ladder_bounce_x`, `ladder_bounce_z` (missing JSON keys → 0)
- No schema v4, no editor/greybox/`kLadderInset` changes, no same-tile step-off
- `rat_core` stays glfw/miniaudio-free; do not commit `imgui.ini`
- Tests from repo root (PowerShell):

```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build "C:\5_gamedev\rat-engine\build" --config Release --target rat_tests && .\build\tests\rat_tests.exe "[filter]"'
```

## File map

- Modify: `src/engine/include/rat/player.hpp` — JumpState bounce fields, JumpTuning ladder fields, `kLadderBounceNudge`, `ignore_ladders` on `integrate_player_surface`
- Modify: `src/engine/src/player.cpp` — skip climb when `ignore_ladders`
- Modify: `src/engine/src/player_jump.cpp` — bounce, lockout tick, bounce XZ, `make_grounded_jump_state` zeros new fields
- Modify: `src/engine/src/replay.cpp` — `mix_jump`
- Modify: `src/engine/src/debug_snapshot.cpp` — dump/load jump
- Modify: `tests/player_jump_test.cpp` — bounce / lockout / remount / ground jump unchanged
- Modify: `tests/debug_snapshot_test.cpp` — roundtrip new fields; missing keys → 0
- Modify: `vault/engine/Systems Index.md` — collision row: jump-off + lockout
- Do not modify: map loader, greybox, `kLadderInset`, editor tools

---

### Task 1: JumpState lockout fields, snapshot, replay mix

**Files:**
- Modify: `src/engine/include/rat/player.hpp`
- Modify: `src/engine/src/player_jump.cpp` (`make_grounded_jump_state` only)
- Modify: `src/engine/src/replay.cpp` (`mix_jump`)
- Modify: `src/engine/src/debug_snapshot.cpp` (`dump_jump` / `load_jump`)
- Test: `tests/debug_snapshot_test.cpp`
- Test: `tests/player_jump_test.cpp` (grounded-state zeros)

**Interfaces:**
- Consumes: existing `JumpState`, `make_grounded_jump_state`, `dump_jump`/`load_jump`, `mix_jump`
- Produces:

```cpp
struct JumpState {
  float jump_offset = 0.0f;
  float vertical_speed = 0.0f;
  float coyote_time_left = 0.0f;
  float jump_buffer_left = 0.0f;
  bool grounded = true;
  int support_blocker_index = -1;
  float ladder_lockout_left = 0.0f;
  float ladder_bounce_x = 0.0f;
  float ladder_bounce_z = 0.0f;
};

struct JumpTuning {
  float gravity = 24.0f;
  float jump_speed = 7.5f;
  float jump_cut = 0.4f;
  float faster_fall_gravity = 42.0f;
  float max_fall_speed = 32.0f;
  float coyote_seconds = 0.1f;
  float input_buffer_seconds = 0.1f;
  float max_substep_seconds = 1.0f / 120.0f;
  float ladder_lockout_seconds = 0.20f;
  float ladder_hop_speed = 4.0f;
  float ladder_bounce_speed = 4.0f;
};

inline constexpr float kLadderBounceNudge = 0.6f;
```

`make_grounded_jump_state` zeros the three new JumpState fields. `mix_jump` mixes them after `support_blocker_index`. Snapshot JSON keys: `ladder_lockout_left`, `ladder_bounce_x`, `ladder_bounce_z`.

- [ ] **Step 1: Write the failing tests**

Append to `tests/player_jump_test.cpp` (after includes / helpers is fine; keep `[unit][player]`):

```cpp
TEST_CASE("make_grounded_jump_state zeros ladder lockout and bounce", "[unit][player]") {
  const rat::JumpState jump = rat::make_grounded_jump_state();
  REQUIRE(jump.ladder_lockout_left == 0.0f);
  REQUIRE(jump.ladder_bounce_x == 0.0f);
  REQUIRE(jump.ladder_bounce_z == 0.0f);
}
```

In `tests/debug_snapshot_test.cpp`, in the existing write/read case that sets `jump.jump_offset = 1.25f`, also set:

```cpp
  jump.ladder_lockout_left = 0.15f;
  jump.ladder_bounce_x = -1.0f;
  jump.ladder_bounce_z = 0.0f;
```

and assert those roundtrip. Add a case that a snapshot JSON object **without** those keys loads as 0 (write a tiny JSON via `read_debug_snapshot` if the loader needs a full snapshot — otherwise extend `load_jump` coverage by writing a snapshot then stripping keys is too heavy; instead add CHECKs that a freshly loaded jump from a file written **before** the new keys still works: write with defaults 0, read, REQUIRE zeros). Minimal extra case: after `make_grounded_jump_state`, `write_debug_snapshot` / `read_debug_snapshot` → lockout and bounce are 0. The 1.25f case must roundtrip the non-zero values.

- [ ] **Step 2: Run tests to verify they fail**

Run: `rat_tests.exe "make_grounded_jump_state zeros ladder lockout"` and the debug snapshot case.

Expected: RED — `JumpState` has no `ladder_lockout_left` (compile) or values do not roundtrip.

- [ ] **Step 3: Add fields, zero them, snapshot, mix**

`player.hpp`: fields as **Interfaces**. `make_grounded_jump_state` sets the three new floats to 0. `dump_jump` / `load_jump` with `node.value(..., 0.0f)`. `mix_jump`:

```cpp
  mix_f32(hash, jump.ladder_lockout_left);
  mix_f32(hash, jump.ladder_bounce_x);
  mix_f32(hash, jump.ladder_bounce_z);
```

Do not change bounce/lockout locomotion yet.

- [ ] **Step 4: Run tests to verify they pass**

Run: `rat_tests.exe "[unit][player],[debug]"` (or the specific snapshot tag in `debug_snapshot_test.cpp`). Expected: PASS. Then `ctest --test-dir build --output-on-failure` or at least `[unit][player]` + snapshot tests.

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/player.hpp src/engine/src/player_jump.cpp src/engine/src/replay.cpp src/engine/src/debug_snapshot.cpp tests/player_jump_test.cpp tests/debug_snapshot_test.cpp
git commit -m "Store ladder lockout and bounce on JumpState for replay and snapshots."
```

Implementer commits **only these files**. Do not set vault Done / In review.

---

### Task 2: Bounce-off, lockout ignore, remount

**Files:**
- Modify: `src/engine/include/rat/player.hpp` — add `bool ignore_ladders = false` as the last parameter of `integrate_player_surface`
- Modify: `src/engine/src/player.cpp` — if `ignore_ladders`, skip the overlapping-ladder climb block
- Modify: `src/engine/src/player_jump.cpp` — bounce + lockout in `integrate_player_frame_surface`
- Test: `tests/player_jump_test.cpp`
- Modify: `vault/engine/Systems Index.md` — collision notes: jump-off bounce + lockout
- Existing: `tests/player_test.cpp` “East ladder climb reaches slab with move only” and “East ladder tangent cannot walk through slab side” must stay green (no edits unless a helper signature force-update)

**Interfaces:**
- Consumes: Task 1 fields; `overlapping_ladder`; `RampDirection`; `kLadderBounceNudge`; `JumpTuning::ladder_*`
- Produces: bounce behavior in `integrate_player_frame_surface`. Face-away helper (file-local in `player_jump.cpp`):

```cpp
void ladder_face_away(RampDirection face, float& ax, float& az) {
  ax = 0.0f;
  az = 0.0f;
  switch (face) {
    case RampDirection::East:
      ax = -1.0f;
      break;
    case RampDirection::West:
      ax = 1.0f;
      break;
    case RampDirection::North:
      az = 1.0f;
      break;
    case RampDirection::South:
      az = -1.0f;
      break;
  }
}
```

**Tick order** (each jump substep, `jump_pressed_this_substep` still `input.jump_pressed && i == 0`):

1. If `ladder_lockout_left <= 0` and `overlapping_ladder` at **start** of substep is non-null **and** (`jump_pressed_this_substep` **or** `jump.jump_buffer_left > 0`): bounce — set `grounded = false`; `vertical_speed = tuning.ladder_hop_speed`; `jump_offset` stays consistent with current air feet (`max(0, player.y - support/ground)` as the rest of the function already does for air); store unit face-away on `ladder_bounce_x/z`; add `kLadderBounceNudge` along that unit to `player.x/z` (then resolve walls/blockers like other XZ moves); set `ladder_lockout_left = tuning.ladder_lockout_seconds`; clear `jump_buffer_left`; **do not** take the climb `continue`. Fall through into the existing air/gravity path for this substep.
2. Else if lockout <= 0 and overlapping: existing climb `continue` (zero vertical_speed, treat as grounded).
3. If lockout > 0: do **not** climb / auto-latch. Tick `ladder_lockout_left = max(0, lockout - step_dt)`. Call `integrate_player_surface(..., ignore_ladders=true)` for WASD **or** apply move the same way the air path already moves XZ. Additionally add `ladder_bounce_x/z * ladder_bounce_speed * step_dt` to XZ (resolve walls). When lockout hits 0, zero `ladder_bounce_x/z`.
4. A hop that is **not** overlapping at start, then enters the volume later in the same frame: latch (step 2), no bounce on that `jump_pressed`.

Wall resolve for nudge/bounce XZ: reuse the overlap/wall revert pattern already used in the ladder tangent block in `player.cpp`.

- [ ] **Step 1: Write the failing tests**

Use schema-2 `make_surface_map` + push slab/ladder like `tests/player_test.cpp` east ladder (tile `{0,0}` east, `y_lo=0`, `y_hi=2`, slab `top_y=2` thickness 0.25). Player start `x=0.85`, `y=1.0`, `z=0.5`, `speed=5`. `JumpTuning` defaults (hop 4, lockout 0.20, bounce 4). Always pass `&map` into `integrate_player_frame_surface`.

```cpp
TEST_CASE("East ladder jump bounces away from face with lockout", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.schema_version = 3;
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.85f;
  body.y = 1.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.body.x < body.x - 0.4f);
  REQUIRE(result.body.y > body.y);
  REQUIRE(result.jump.ladder_lockout_left == Catch::Approx(0.20f).margin(1.0f / 120.0f));
  REQUIRE(result.jump.grounded == false);
  REQUIRE(result.jump.vertical_speed == Catch::Approx(4.0f).margin(0.5f));
}

TEST_CASE("Ladder lockout ignores overlap then remounts", "[unit][player]") {
  // same map + start as above
  // 1) jump_pressed one frame
  // 2) ~0.15s more with no jump, no move — y must not clamp to ladder (can fall/hop, must not stick as grounded climb)
  //    REQUIRE lockout still > 0 after 0.15s; REQUIRE jump.grounded == false or y not frozen at 1.0 via climb
  // 3) tick remaining lockout to 0 while keeping x,z inside the volume (hold toward +X if needed after lockout)
  //    After lockout 0 and overlap: next frame without jump remounts (grounded, vertical_speed 0)
}

TEST_CASE("Ground jump not overlapping a ladder still uses jump_speed", "[unit][player]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  const rat::PlayerFrameResult result = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(result.jump.vertical_speed == Catch::Approx(7.5f).margin(0.05f));
  REQUIRE(result.jump.ladder_lockout_left == 0.0f);
}

TEST_CASE("Jump that enters a ladder after takeoff latches instead of bouncing", "[unit][player]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
  map.schema_version = 3;
  map.ladders.push_back({{1, 0}, rat::RampDirection::West, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.3f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.speed = 5.0f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  input.move.axis_x = 1.0f;
  const rat::PlayerFrameResult first = rat::integrate_player_frame_surface(
      body, jump, input, 1.0f / 120.0f, {}, query, {}, 0.35f, {}, &map);
  REQUIRE(first.jump.ladder_lockout_left == 0.0f);
  REQUIRE(first.jump.vertical_speed == Catch::Approx(7.5f).margin(0.05f));
}
```

Fill the lockout/remount case so it is executable (same map helper as the bounce case). After bounce, run ~18 frames at 120 Hz (0.15s) with empty input; REQUIRE `ladder_lockout_left > 0` and `grounded == false`. Then run frames until lockout is 0 (about 6 more). If the body left the volume, move +X back to `x=0.85` with lockout already 0 (or place so bounce leaves then walk back). After overlap + lockout 0 + no jump, REQUIRE remount: `grounded` and `vertical_speed == 0`.

Keep “East ladder climb reaches slab with move only” unchanged and green.

- [ ] **Step 2: Run tests to verify they fail**

Run: `rat_tests.exe "East ladder jump bounces"` — Expected: RED (jump swallowed, x unchanged / lockout 0).

- [ ] **Step 3: Implement bounce and lockout**

Follow **Tick order**. Pass `ignore_ladders=true` into `integrate_player_surface` when lockout > 0. Gravity applies during lockout (air path). Do not change `kLadderInset` or greybox.

Systems Index collision cell: mention jump-off (face bounce + lockout); keep existing bake/climb sentence.

- [ ] **Step 4: Run tests to verify they pass**

Run: `rat_tests.exe "[unit][player]"` and `"[unit][player][surface]"` (player_test ladder cases). Expected: PASS including east climb move-only and tangent-vs-slab. Then full `ctest --test-dir build --output-on-failure`.

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/player.hpp src/engine/src/player.cpp src/engine/src/player_jump.cpp tests/player_jump_test.cpp vault/engine/Systems Index.md
git commit -m "Bounce off a ladder on jump so overlap does not remount during lockout."
```

Do not set vault Done / In review.

---

## Self-review (plan vs spec)

| Spec | Task |
| --- | --- |
| Into +Y / away −Y | unchanged climb path (Task 2 step 2/4) |
| Overlap latches | Task 2 remount + existing climb test |
| Bounce hop 4, nudge 0.6, xz 4, lockout 0.20 | Task 2 bounce test + tick |
| Face-away signs | `ladder_face_away` |
| Buffer while latched = bounce | Task 2 tick order step 1 |
| Enter-later same frame no bounce | Task 2 tick order step 4 |
| Snapshot + mix | Task 1 |
| No schema/editor/step-off | file map exclusions |
