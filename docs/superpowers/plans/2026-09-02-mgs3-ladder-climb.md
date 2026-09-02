# MGS3 Ladder Climb Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Full MGS3 ladders: Interact mount, rail climb, camera behind the rungs, no jump-off, step off at top/bottom.

**Architecture:** Climb is only `tick_climb_substep` in `player_jump.cpp`. `integrate_player_surface` never climbs. Camera pose is pure `rat_core`; greybox applies it while `climbing`. Interact is consumed by the player tick if the result is climbing.

**Tech Stack:** C++20, Catch2, `rat_core` / `rat_engine`. Windows: VsDevCmd x64 + `build/` Ninja Release.

## Global Constraints

- Spec: `docs/superpowers/specs/2026-09-02-mgs3-ladder-climb-design.md`
- Vault: `vault/production/tasks/feat-mgs3-ladder-climb.md` — implementer does **not** set Done / In review
- Do not change `camera_relative_move` math except using it in tests
- No schema v4; bake / `kLadderInset` unchanged
- `rat_core` stays glfw/miniaudio-free; do not commit `imgui.ini`
- Tests:

```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build "C:\5_gamedev\rat-engine\build" --config Release --target rat_tests && .\build\tests\rat_tests.exe "[filter]"'
```

## File map

- Modify: `src/engine/include/rat/camera.hpp`, `src/engine/src/camera.cpp` — `climb_camera_pose`
- Modify: `src/engine/include/rat/collision.hpp`, `src/engine/src/collision.cpp` — `ladder_face_into`
- Modify: `src/engine/include/rat/player.hpp`, `src/engine/src/player.cpp`, `src/engine/src/player_jump.cpp`, `src/engine/src/input.cpp`
- Modify: `src/engine/src/simulation_session.cpp`, `src/engine/src/replay.cpp`, `src/engine/src/debug_snapshot.cpp`
- Modify: `src/engine/include/rat/greybox.hpp`, `src/engine/src/greybox.cpp`, `apps/editor/editor_app.cpp`
- Test: `tests/camera_test.cpp`, `tests/player_jump_test.cpp`, `tests/player_test.cpp`, `tests/locomotion_test.cpp`, `tests/debug_snapshot_test.cpp`, `tests/input_test.cpp` as needed
- Vault: Systems Index, GPP, MGS3 card (status left In progress)

---

### Task 1: Climb camera pose + face into

**Files:**
- Modify: `src/engine/include/rat/camera.hpp`
- Modify: `src/engine/src/camera.cpp`
- Modify: `src/engine/include/rat/collision.hpp`
- Modify: `src/engine/src/collision.cpp`
- Test: `tests/camera_test.cpp`
- Test: `tests/collision_test.cpp` (one face-into case) or put into-face checks in camera_test by passing +X

**Interfaces:**
- Produces:

```cpp
inline constexpr float kClimbCameraBack = 10.0f;
inline constexpr float kClimbCameraHeight = 8.0f;
struct ClimbCameraPose { Vec3 eye{}; Vec3 focus{}; };
[[nodiscard]] ClimbCameraPose climb_camera_pose(Vec3 player, float into_x, float into_z);

[[nodiscard]] void ladder_face_into(RampDirection face, float& into_x, float& into_z);
// East +1,0; West -1,0; North 0,-1; South 0,+1
```

- [ ] **Step 1: Failing tests**

In `tests/camera_test.cpp`:

```cpp
TEST_CASE("Climb camera sits behind the player looking into an east face", "[unit][camera]") {
  const rat::Vec3 player{0.85f, 1.0f, 0.5f};
  const rat::ClimbCameraPose pose = rat::climb_camera_pose(player, 1.0f, 0.0f);
  REQUIRE(pose.focus.x == Approx(player.x));
  REQUIRE(pose.focus.y == Approx(player.y));
  REQUIRE(pose.focus.z == Approx(player.z));
  REQUIRE(pose.eye.x < player.x - 1.0f);
  REQUIRE(pose.eye.y > player.y);
  const rat::MoveInput w =
      rat::camera_relative_move(0.0f, 1.0f, pose.eye, pose.focus);
  REQUIRE(w.axis_x > 0.5f);
}
```

`camera_relative_move` is in `player.hpp` — include it from the test.

- [ ] **Step 2: Run RED**

`.\build\tests\rat_tests.exe "Climb camera sits behind"`

Expected: compile fail or FAIL — `climb_camera_pose` missing.

- [ ] **Step 3: Implement**

Normalize into XZ; if length < 1e-6 use (1,0).  
`eye = {player.x - into_x * kClimbCameraBack, player.y + kClimbCameraHeight, player.z - into_z * kClimbCameraBack}`, `focus = player`.

`ladder_face_into` switch on `RampDirection` in `collision.cpp`. Add a tiny `[collision]` test: East → (1,0).

- [ ] **Step 4: GREEN** `[unit][camera]` and the new collision case.

- [ ] **Step 5: Commit** `Add a climb camera pose behind the player looking at the rungs.`

---

### Task 2: Interact mount, rail climb, no bounce, dismount

**Files:**
- Modify: `src/engine/include/rat/player.hpp` — `climb_into_x/z` on `JumpState`; `interact_pressed` on `PlayerFrameInput`; drop climb from `integrate_player_surface` contract comments if needed
- Modify: `src/engine/src/player.cpp` — delete or permanently skip the overlapping-ladder climb block (always behave as `ignore_ladders`)
- Modify: `src/engine/src/player_jump.cpp` — motor: Climb only if `was_climbing` or interact+mount zone; `tick_climb_substep` snap + 1D Y; jump ignored; dismount top/bottom per spec; **remove bounce/want_bounce/lockout climb path**; remount-on-overlap gone
- Modify: `src/engine/src/input.cpp` — copy `interact_pressed` in `player_input_from_frame`
- Modify: `src/engine/src/simulation_session.cpp` — pass interact into `PlayerFrameInput`; if `jump_.climbing` after integrate, `events_.update(..., false, dt)` for that interact
- Modify: `src/engine/src/replay.cpp` / `debug_snapshot.cpp` — mix/dump `climb_into_x/z` (missing → 0)
- Test: rewrite ladder tests in `tests/player_jump_test.cpp`, `tests/player_test.cpp`, `tests/locomotion_test.cpp`

**Interfaces:**
- Mount helper in anonymous namespace of `player_jump.cpp`:
  - `ladder_face_into` from Task 1
  - overlap or approach at `y_lo` or grounded at `y_hi` as spec
- `tick_climb_substep`: snap to `(min+max)/2` XZ; `axis = move.axis_x * into_x + move.axis_z * into_z` (prefer `climb_move` if nonzero); Y += axis * speed * dt; then dismount checks
- `make_grounded_jump_state` zeros `climb_into_*`

- [ ] **Step 1: Failing tests** (replace bounce / auto-latch cases)

```cpp
TEST_CASE("Overlap without interact does not climb", "[unit][player]") {
  // east ladder map as existing bounce fixture, body at 0.85,1,0.5
  // integrate 1/120 with move into face, interact false
  REQUIRE_FALSE(result.jump.climbing);
}

TEST_CASE("Interact on an east ladder mounts the rail", "[unit][player]") {
  input.interact_pressed = true;
  REQUIRE(result.jump.climbing);
  REQUIRE(result.jump.climb_into_x == Approx(1.0f));
  REQUIRE(result.body.x == Approx(0.5f * (/*volume center — use body after snap, x in (0.7,1.0)*/)).margin(0.2f));
}

TEST_CASE("Jump while climbing does not bounce", "[unit][player]") {
  // mount first, then jump_pressed
  REQUIRE(result.jump.climbing);
  REQUIRE(result.jump.ladder_lockout_left == 0.0f);
}

TEST_CASE("East climb up with into-face move raises Y", "[unit][player]") {
  // mount, then several frames move.axis_x = 1
  REQUIRE(result.body.y > 1.0f);
}

TEST_CASE("Top of east ladder plus up steps onto the same-tile slab", "[unit][player]") {
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  // mount, climb to y_hi with into-face, then more into-face
  REQUIRE_FALSE(result.jump.climbing);
  REQUIRE(result.body.y == Approx(2.0f).margin(0.1f));
  REQUIRE(result.body.x < 0.85f);  // face-away onto the tile
}
```

Update `East ladder climb uses camera steer` to go through `integrate_player_frame_surface` + interact, then `climb_move` / pose W. Update locomotion east-ladder classify to interact first. Delete or rewrite bounce/lockout remount tests.

- [ ] **Step 2: RED** `*ladder*` / new case names.

- [ ] **Step 3: Implement** per spec. Ground motor: `ignore_ladders = true` always. No `apply_ladder_bounce` calls.

- [ ] **Step 4: GREEN** `[unit][player]`, `[unit][loco]`, `[unit][debug]`, `[unit][input]`. Full `ctest` before commit.

- [ ] **Step 5: Commit** `Treat ladders as an Interact rail: no auto-latch, no bounce, step off at the ends.`

---

### Task 3: Greybox climb camera lock

**Files:**
- Modify: `src/engine/include/rat/greybox.hpp` — `set_climb_lock(bool, float into_x, float into_z)`
- Modify: `src/engine/src/greybox.cpp` — in `rebuild_camera` / `draw` after focusing the player, if lock: set `camera_.eye` from `climb_camera_pose`, `bx::mtxLookAt` with up +Y
- Modify: `apps/editor/editor_app.cpp` — **before** `map_input_frame`, if `session_.jump().climbing` call `set_climb_lock(true, climb_into_*)`; else clear. After tick, `set_player` as today (draw also locks).
- Modify: `src/engine/include/rat/engine.hpp` / `engine.cpp` only if you proxy the call; otherwise editor talks to `greybox()` directly.

**Interfaces:**
- Consumes: `climb_camera_pose`, `JumpState.climbing`, `climb_into_x/z`

- [ ] **Step 1:** No glfw test required. If `rat_engine` has no greybox unit test, add a `rat_core` test already in Task 1. Optionally assert in an editor_logic test only if one already constructs GreyboxScene (skip if not — do not add glfw tests).

- [ ] **Step 2:** Implement lock/clear. When unlocked, existing `build_ortho_camera` path unchanged.

- [ ] **Step 3:** Build `rat-editor` Release.

- [ ] **Step 4: Commit** `Lock the greybox camera behind the player while climbing.`

---

### Task 4: Hub notes

**Files:**
- `vault/engine/Systems Index.md`
- `vault/engine/Game Programming Patterns.md` if it still describes bounce/camera-steer climb
- `vault/production/tasks/feat-step-off-ladder-onto-same-tile-floor.md` — point at MGS3 card (do not mark Done unless this slice actually steps onto the slab; if Task 2 shipped that, set Done + Resolution + `Bugs found: none.` after parent review — **implementer leaves MGS3 card In progress**)

- [ ] **Step 1–3:** Collision/locomotion rows: Interact rail, climb camera lock, no bounce.
- [ ] **Step 4: Commit** `Document MGS3 ladder climb on the hub.`

---

## After all tasks

Parent: MGS3 card `In review` → reviewer → Done + Resolution. Rebuild full Release `rat-editor.exe` for hand test (`rebuild-after-stage-close`).
