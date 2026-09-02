# Camera-Aligned Walk Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Play WASD follows the camera via `camera_relative_move`; degenerate/top-down cameras keep today’s W=−Z numbers.

**Architecture:** One-line change in `map_input_frame`: set `frame.move` the same way `climb_move` is already set. `camera_relative_move` / `world_aligned_move` stay; integrate does not change.

**Tech Stack:** C++20, Catch2, `rat_core`. Windows: VsDevCmd x64 + existing `build/` Ninja Release.

## Global Constraints

- Spec: `docs/superpowers/specs/2026-09-02-camera-aligned-walk-design.md`
- Vault: `vault/production/tasks/feat-camera-aligned-walk.md` — implementer does **not** set Done / In review
- Do not change `camera_relative_move` math
- Do not change `integrate_player` / jump / climb motors
- Do not start [[feat: MGS3 ladder climb]] in this plan
- `rat_core` stays glfw/miniaudio-free; do not commit `imgui.ini`
- Tests from repo root (PowerShell):

```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build "C:\5_gamedev\rat-engine\build" --config Release --target rat_tests && .\build\tests\rat_tests.exe "[unit][input]"'
```

## File map

- Modify: `src/engine/src/input.cpp` — `frame.move = camera_relative_move(...)`
- Modify: `src/engine/include/rat/input.hpp` — comment on `InputFrame.move` / `climb_move`
- Modify: `src/engine/include/rat/player.hpp` — comment on `world_aligned_move` (still for tests / fixed axes)
- Test: `tests/input_test.cpp`
- Modify: `vault/engine/Systems Index.md` — Player locomotion / input row: Play walk is camera-relative
- Modify: `vault/production/bugs/camera-switch-changes-wasd-world-directions.md` — Play superseded
- Modify: `vault/production/tasks/feat-mgs3-ladder-climb.md` — walk no longer world-aligned
- Do not modify: `player.cpp` mapping functions, `player_jump.cpp`, greybox camera

---

### Task 1: Play `map_input_frame` uses camera-relative walk

**Files:**
- Modify: `src/engine/src/input.cpp`
- Modify: `src/engine/include/rat/input.hpp`
- Modify: `src/engine/include/rat/player.hpp`
- Test: `tests/input_test.cpp`

**Interfaces:**
- Consumes: `camera_relative_move(float, float, Vec3, Vec3)`, `world_aligned_move(float, float)`, existing `map_input_frame`
- Produces: `InputFrame.move` is camera-relative when eye/focus have a look XZ; degenerate camera matches `world_aligned_move`

- [ ] **Step 1: Rewrite the input tests**

Replace `TEST_CASE("InputFrame maps WASD to world-aligned move"` and `TEST_CASE("InputFrame climb_move follows camera while walk stays world-aligned"` with:

```cpp
TEST_CASE("InputFrame default camera walk matches world-aligned fallback", "[unit][input]") {
  rat::InputButtons down;
  down.move_up = true;
  down.move_right = true;
  rat::InputGating gate;

  const rat::InputFrame frame = rat::map_input_frame(down, {}, gate);
  const rat::MoveInput expected = rat::world_aligned_move(1.0f, 1.0f);
  CHECK(frame.move.axis_x == expected.axis_x);
  CHECK(frame.move.axis_z == expected.axis_z);
  CHECK(frame.climb_move.axis_x == frame.move.axis_x);
  CHECK(frame.climb_move.axis_z == frame.move.axis_z);
}

TEST_CASE("InputFrame walk follows camera look", "[unit][input]") {
  rat::InputButtons down;
  down.move_up = true;
  rat::InputGating gate;
  const rat::Vec3 eye{0.0f, 8.0f, 0.0f};
  const rat::Vec3 focus{4.0f, 0.0f, 0.0f};

  const rat::InputFrame frame = rat::map_input_frame(down, {}, gate, eye, focus);
  const rat::MoveInput cam_w = rat::camera_relative_move(0.0f, 1.0f, eye, focus);
  CHECK(frame.move.axis_x == cam_w.axis_x);
  CHECK(frame.move.axis_z == cam_w.axis_z);
  CHECK(frame.climb_move.axis_x == frame.move.axis_x);
  CHECK(frame.climb_move.axis_z == frame.move.axis_z);
  CHECK(frame.move.axis_x > 0.5f);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `.\build\tests\rat_tests.exe "InputFrame walk follows camera look"`

Expected: FAIL — `frame.move.axis_x` is `0` (world W=−Z), not `cam_w.axis_x` > 0.5.

- [ ] **Step 3: Map walk through the camera**

In `src/engine/src/input.cpp`, inside the `allow_player` block, replace the two move lines with:

```cpp
    frame.move = camera_relative_move(screen_x, screen_z, camera_eye, camera_focus);
    frame.climb_move = frame.move;
```

Comment on `InputFrame` in `src/engine/include/rat/input.hpp`:

```cpp
  MoveInput move{};        // Play: camera-relative (degenerate camera = world-aligned fallback)
  MoveInput climb_move{};  // same mapping until MGS3 1D climb
```

Comment on `world_aligned_move` in `src/engine/include/rat/player.hpp`: keep the helper; Play input no longer uses it.

- [ ] **Step 4: Run tests to verify they pass**

Run: `.\build\tests\rat_tests.exe "[unit][input]"`

Expected: All `[unit][input]` cases PASS.

- [ ] **Step 5: Commit**

```powershell
git add src/engine/src/input.cpp src/engine/include/rat/input.hpp src/engine/include/rat/player.hpp tests/input_test.cpp
git commit -m "Map Play WASD through the camera so W is into the shot."
```

---

### Task 2: Hub notes — Play walk follows camera

**Files:**
- Modify: `vault/engine/Systems Index.md` (Player locomotion and/or Collision/input wording)
- Modify: `vault/production/bugs/camera-switch-changes-wasd-world-directions.md`
- Modify: `vault/production/tasks/feat-mgs3-ladder-climb.md`
- Modify: `vault/production/tasks/feat-camera-aligned-walk.md` — implementer still leaves status In progress; parent sets In review

**Interfaces:**
- Consumes: Task 1 behavior
- Produces: vault text matches Play camera-relative walk; MGS3 card no longer says walk is world-aligned

- [ ] **Step 1: Update Systems Index**

On the Player locomotion row, add that Play `InputFrame.move` is `camera_relative_move` (C rotates W). Edit has no player WASD.

- [ ] **Step 2: Annotate the old camera-switch bug**

At the bottom of `vault/production/bugs/camera-switch-changes-wasd-world-directions.md` add:

```markdown
Follow-up: Play walk is camera-relative again — [[feat: Camera-aligned walk]]. `C` rotating W in Play is intended. Edit still has no player WASD.
```

Leave `status: Fixed`.

- [ ] **Step 3: Point the MGS3 card at this**

In `feat-mgs3-ladder-climb.md` Acceptance, replace “Ходьба вне лестницы — world-aligned” with “Ходьба в Play — camera-aligned ([[feat: Camera-aligned walk]]); на Climb камера локается так, что вперёд = вверх.”

Add `Depends: [[feat: Camera-aligned walk]]` if missing.

- [ ] **Step 4: Commit**

```powershell
git add vault/engine/"Systems Index.md" vault/production/bugs/camera-switch-changes-wasd-world-directions.md vault/production/tasks/feat-mgs3-ladder-climb.md
git commit -m "Document Play camera-aligned walk on the hub."
```

Do not set the camera-walk card to Done.

---

## Execution

After Task 1+2: parent sets the vault card `In review`, then a read-only reviewer. Full `ctest` once `[unit][input]` is green. Then [[feat: MGS3 ladder climb]] can assume W is into the shot.
