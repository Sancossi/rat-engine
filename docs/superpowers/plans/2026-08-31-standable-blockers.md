# Standable Jumpable Blockers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make jumpable blocker tops stable player supports without regressing raised terrain traversal.

**Architecture:** Store the active blocker support index in `JumpState`. Player
integration selects blocker `top_y` as effective ground only after a swept
landing from above, validates the support each substep, and converts support
loss into the existing coyote-fall path.

**Tech Stack:** C++20, Catch2, CMake/Ninja.

## Global Constraints

- Keep `SurfaceQuery` responsible for authored terrain and ramps.
- Blocker support is acquired only by descending through `top_y` from above.
- Existing side collision and jump-over behavior must remain.
- Use test-first RED/GREEN and do not commit unrelated workspace changes.

---

### Task 1: Blocker-top support

**Files:**
- Modify: `src/engine/include/rat/player.hpp`
- Modify: `src/engine/src/player_jump.cpp`
- Modify: `tests/player_jump_test.cpp`

**Interfaces:**
- Consumes: `BlockerDef::{bounds, top_y, jumpable}` and existing jump integration.
- Produces: `JumpState::support_blocker_index` and stable blocker-top movement.

- [ ] **Step 1: Write failing behavior tests**

Add Catch2 cases that descend onto a jumpable blocker, remain grounded and idle
at `top_y`, move on top, walk off with coyote time, jump from the top, retain
side blocking below the top, and fully clear the blocker when horizontal
momentum carries the body beyond its footprint.

- [ ] **Step 2: Verify RED**

Run:
`build\tests\rat_tests.exe "[unit][player][jump][support]"`

Expected: landing support assertions fail because current code ejects the body.

- [ ] **Step 3: Implement minimal support state**

Add an invalid-by-default blocker index to `JumpState`. In each fixed substep:
validate current support, use its `top_y` as effective ground, acquire support
on a descending swept top crossing, clear it on jump, and preserve feet Y plus
coyote time when walking off.

- [ ] **Step 4: Verify support and terrain regressions**

Run:
`build\tests\rat_tests.exe "[unit][player]"`

Expected: all player tests pass, including raised terrain and jump-over cases.

- [ ] **Step 5: Build and run the full suite**

Run through the Visual Studio developer environment:
`cmake --build build --config Release --target rat_tests rat-editor`
then:
`ctest --test-dir build -C Release --output-on-failure`

Expected: build succeeds and all tests pass.
