# Bake Terrain Collision Solids Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bake height-grid/ramp side faces into `CollisionWorld` so a +1.0 cube blocks the player cylinder from the side without breaking stairs or ramp walking.

**Architecture:** Convert `build_terrain_side_faces` into `FenceSolid`s (min/max Y of the trapezoid). Same circle-vs-segment test as fences, plus skip if wall height ≤ `max_step_up` (0.35) or feet ≥ `y_hi`. `SurfaceQuery.sample` still sets standing Y.

**Tech Stack:** C++20, Catch2, `rat_core`. VsDevCmd x64; cmake from VS BuildTools if needed.

## Global Constraints

- Cylinder radius **0.4**, height **1.6**; XZ circle.
- Terrain walls from `build_terrain_side_faces` only — not filled tile AABBs, not ramp-as-support.
- Skip wall if `(y_hi - y_lo) <= max_step_up` with default **0.35**.
- Skip wall if `feet + 1e-4 >= y_hi`.
- `SurfaceQuery.sample` remains for standing Y.
- Optional `const MapData*` on integrate; null = fences only. Editor passes the map.
- No ECS, crates, stacked floors, signature break of existing defaulted call sites.
- Card stays `In progress`. Do not set `Done` / `In review`.
- Do not commit `imgui.ini`.

## File map

- Modify: `src/engine/include/rat/collision.hpp`, `src/engine/src/collision.cpp`, `tests/collision_test.cpp`
- Modify: `src/engine/include/rat/player.hpp`, `src/engine/src/player.cpp`, `src/engine/src/player_jump.cpp`
- Modify: `apps/editor/editor_app.cpp`, `tests/player_test.cpp`
- Modify: `src/engine/src/input_sequence.cpp` if it has a map pointer available
- Modify: `vault/engine/Systems Index.md` note (terrain walls)

---

### Task 1: Bake terrain side faces and hit rules

**Files:**
- Modify: `src/engine/include/rat/collision.hpp`
- Modify: `src/engine/src/collision.cpp`
- Modify: `tests/collision_test.cpp`

**Interfaces:**
- Consumes: `build_terrain_geometry`, `build_terrain_side_faces`, existing `FenceSolid` / `circle_hits_segment`
- Produces:
  - `void append_terrain_walls(CollisionWorld& world, const HeightGrid& grid, std::span<const RampDef> ramps, float tile_size);`
  - `CollisionWorld bake_collision_world(const MapData& map, const SurfaceQuery& query);` — `bake_fence_world(map.edge_barriers, query)` then `append_terrain_walls`
  - `bool cylinder_hits_walls(const CollisionBody& body, const CollisionWorld& world, float max_step_up = 0.35f);` — fences **and** terrain solids in `world.fences` (append into the same vector). Skip a solid if `body.y + kFenceFeetClearanceEpsilon >= y_hi` OR `(y_hi - y_lo) <= max_step_up`. Then Y overlap + circle vs segment as today.
  - Keep `cylinder_hits_fences` as `cylinder_hits_walls(body, world, /*max_step_up=*/0.0f)` **or** keep the old function for fence-only tests (0.45 fences must still hit: span 0.45 > 0.0 if using 0, but 0.45 > 0.35 so they also hit with 0.35). **Fence Mini 0.45 must still block at feet=0.** 0.45 > 0.35 → blocked. Good. Jump-over: feet >= 0.45 skip remains.

- [ ] **Step 1: Failing tests** `[collision]`:
  1. Map 2×1, ground `{0, 1}`: bake_collision_world has a wall on x=1 with `(y_hi-y_lo)==1` (Approx).
  2. Body at (0.7, 0, 0.5) hits that world with max_step_up 0.35; body at (0.7, 1, 0.5) does not.
  3. Map 2×1, ground `{0, 0.25}`: body at (0.7, 0, 0.5) does **not** hit (span ≤ 0.35).
  4. Existing east 0.45 fence tests still pass with `cylinder_hits_walls(..., 0.35f)`.

- [ ] **Step 2: RED** (missing `bake_collision_world` / hit).

- [ ] **Step 3: Implement** append from `TerrainSideFace`: `ax,az,bx,bz` = x0,z0,x1,z1; `y_lo = min(y0_lo,y1_lo)`, `y_hi = max(y0_hi,y1_hi)`; skip degenerate `y_lo==y_hi` (already omitted by maybe_push).

- [ ] **Step 4: `[collision]` green.**

- [ ] **Step 5: Commit** collision.hpp/cpp + collision_test.cpp.

---

### Task 2: Wire MapData into integrate and editor; cube side-walk test

**Files:**
- Modify: `src/engine/include/rat/player.hpp` — add `const MapData* map = nullptr` as the **last** parameter of `integrate_player`, `integrate_player_surface`, `integrate_player_frame_surface`.
- Modify: `src/engine/src/player.cpp` — if `map != nullptr`, `bake_collision_world(*map, query)`; else `bake_fence_world`. Hit with `cylinder_hits_walls(body, world, max_step_up)` (legacy `integrate_player` uses 0.35f when query present).
- Modify: `src/engine/src/player_jump.cpp` — thread `map` through to integrate calls.
- Modify: `apps/editor/editor_app.cpp` — pass `&events_.map()`.
- Modify: `src/engine/src/input_sequence.cpp` — pass `&map` if in scope.
- Modify: `tests/player_test.cpp` — side-walk along a 1.0 cube: 2×2 or 2×1 grid, high tile (1,0) y=1, player on (0,0) at x=0.5, z=-0.3, walk +Z, pass `&map`. REQUIRE z stays < 0 (cannot slide through the west face of the cube). Keep existing stair/ramp tests **without** map pointer (behavior unchanged). Add one stair test **with** map pointer that still walks up 0.25.

**Interfaces:**
- Consumes: Task 1 `bake_collision_world`, `cylinder_hits_walls`
- Produces: editor Play uses terrain walls

- [ ] **Step 1: Failing cube side-walk test** with `&map`.

- [ ] **Step 2: RED.**

- [ ] **Step 3: Wire signatures and editor.**

- [ ] **Step 4: `[player][surface]`, `[collision]`, `[player][edge]`, full `rat_tests`.** Ramp low-edge tests without map stay green; with map, ramp side faces taller than 0.35 may block **side** entry — that is intended. If a **low-edge** ramp test fails because a face was baked on the entry, fix bake/skip (do not block the low edge: that face has ~0 height).

- [ ] **Step 5: Systems Index one-line. Commit.** Not Done on the vault card.

---

## Spec coverage

| Spec | Task |
| --- | --- |
| Bake side faces | 1 |
| Skip ≤ max_step_up / feet ≥ top | 1 |
| Cube side-walk blocked | 2 |
| Stairs still walkable | 1+2 |
| Sample still for Y | 2 |
| Editor passes map | 2 |
