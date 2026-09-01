# Collision World Fence Solids Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Player cylinder (radius 0.4, height 1.6) cannot walk through baked fence solids from the side; `blocked_by_edge_barriers` center-cross is removed.

**Architecture:** New `rat_core` `CollisionWorld` holds fence quads baked from `edge_barriers` + `SurfaceQuery`. Integrate X-then-Z tests `cylinder_hits_fences` after each axis. Blockers stay square AABB. Ground stays `SurfaceQuery.sample` at feet center. No ECS.

**Tech Stack:** C++20, Catch2, `rat_core` / `rat_tests`. Windows: VsDevCmd x64 if cl missing; `cmake --build build --target rat_tests`.

## Global Constraints

- Cylinder radius **0.4** (`PlayerBody::half_extent`); height **1.6** from feet; XZ is a **circle**.
- Fence solid: XZ segment on the owner-tile edge; Y `[owner_top, owner_top+height]`; `owner_top` = `SurfaceQuery.sample` at tile center.
- If `feet + 1e-4 >= owner_top + height`, fence does not block (Mini 0.45 jumpable).
- Replace `blocked_by_edge_barriers`; do not keep the center-cross test.
- Do not bake height-grid cubes or ramp wedges; no crates, caves, ECS, physics engine.
- Keep `integrate_player*` signatures (`edge_barriers` + `SurfaceQuery`); bake inside the call.
- Tests: Catch2 tags `[collision]` and existing `[edge]` / `[player]`. Headless, no window.
- Vault bug stays `Investigating`; do not set `Fixed` / `In review`.
- Do not commit `imgui.ini` or unrelated vault noise.

## File map

- Create: `src/engine/include/rat/collision.hpp`, `src/engine/src/collision.cpp`, `tests/collision_test.cpp`
- Modify: `src/engine/CMakeLists.txt`, `tests/CMakeLists.txt`, `src/engine/src/player.cpp`, `src/engine/include/rat/player.hpp` (only if a helper must be public), `src/engine/src/event_runtime.cpp`, `tests/player_test.cpp`, `vault/engine/Systems Index.md`

---

### Task 1: CollisionWorld bake and cylinder vs fence

**Files:**
- Create: `src/engine/include/rat/collision.hpp`
- Create: `src/engine/src/collision.cpp`
- Create: `tests/collision_test.cpp`
- Modify: `src/engine/CMakeLists.txt` (add `src/collision.cpp` to `rat_core`)
- Modify: `tests/CMakeLists.txt` (add `collision_test.cpp` to `rat_tests`)

**Interfaces:**
- Consumes: `MapData` / `EdgeBarrierDef` / `RampDirection` (`rat/map_data.hpp`), `SurfaceQuery`, `PlayerBody`
- Produces:
  - `constexpr float kPlayerCylinderHeight = 1.6f;`
  - `constexpr float kFenceFeetClearanceEpsilon = 1e-4f;`
  - `struct FenceSolid { float ax, az, bx, bz, y_lo, y_hi; };`
  - `struct CollisionBody { float x, y, z; float radius = 0.4f; float height = kPlayerCylinderHeight; float mass = 1.0f; float vel_x = 0, vel_y = 0, vel_z = 0; };`
  - `struct CollisionWorld { std::vector<FenceSolid> fences; };`
  - `CollisionBody collision_body_from_player(const PlayerBody& player);` — copies x/y/z, `radius = player.half_extent`, `height = kPlayerCylinderHeight`, mass 1, velocity 0
  - `CollisionWorld bake_fence_world(std::span<const EdgeBarrierDef> barriers, const SurfaceQuery& query);`
  - `bool cylinder_hits_fences(const CollisionBody& body, const CollisionWorld& world);`

- [ ] **Step 1: Write the failing tests** in `tests/collision_test.cpp` (no implementation yet). Include `<rat/collision.hpp>`, `<rat/map_data.hpp>`, `<rat/surface_query.hpp>`, Catch2.

Helper in the test file:

```cpp
rat::MapData make_grid(int w, int h, float ground = 0.0f) {
  rat::MapData map;
  map.schema_version = 2;
  map.tile_size = 1.0f;
  map.width = w;
  map.height = h;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = w;
  map.height_grid.height = h;
  map.height_grid.ground_y.assign(static_cast<std::size_t>(w * h), ground);
  return map;
}
```

Cases (tag `[collision]`):

1. Bake east fence on tile (0,0) height 0.45: world has one fence; segment x=1, z in [0,1]; `y_lo==0`, `y_hi==0.45`.
2. `collision_body_from_player` at (0.5,0,0.5) `half_extent=0.4`: radius 0.4, height 1.6.
3. Body at (0.5,0,0.5) does **not** hit (center 0.5 from east edge, radius 0.4). Body at (0.7,0,0.5) **does** hit.
4. Body at (1.3, 0, 0.5) hits (adjacent tile, center never on the other side of x=1).
5. Body at (0.7, 0.45, 0.5) does **not** hit 0.45 fence (`feet + eps >= y_hi`).
6. Skip `height <= 0` barriers (empty world).

- [ ] **Step 2: Run tests — must fail to compile** (missing header). Then after adding empty decls, geometric cases fail until implemented.

Run: `cmake --build build --target rat_tests` then `.\build\tests\rat_tests.exe "[collision]"`

- [ ] **Step 3: Implement** `collision.hpp` / `collision.cpp`.

Bake: skip `height <= 0`. Tile origin `ox = tile.x * ts`, `oz = tile.z * ts`. `owner_top = query.sample(ox + 0.5*ts, oz + 0.5*ts).y`. Segment by `RampDirection`: East `(ox+ts,oz)-(ox+ts,oz+ts)`, West `(ox,oz)-(ox,oz+ts)`, South `(ox,oz+ts)-(ox+ts,oz+ts)`, North `(ox,oz)-(ox+ts,oz)`. `y_lo = owner_top`, `y_hi = owner_top + height`.

Hit: if `body.y + kFenceFeetClearanceEpsilon >= solid.y_hi` continue. If Y ranges `[body.y, body.y+body.height]` and `[y_lo, y_hi]` do not overlap, continue. Else circle vs segment in XZ: closest point on segment, dist2 <= radius^2.

- [ ] **Step 4: `[collision]` all pass.**

- [ ] **Step 5: Commit** only collision files + CMakeLists. Message: why cylinder vs baked fence segment, not center-cross.

---

### Task 2: Integrate movement and side-walk regression

**Files:**
- Modify: `src/engine/src/player.cpp` — delete `blocked_by_edge_barriers`; after each axis step, if `surface_query` and barriers, `bake_fence_world` once per `integrate_*` call (not per substep), then `cylinder_hits_fences(collision_body_from_player(player), world)`. Keep `overlaps_any` / `player_bounds` for blockers (square AABB).
- Modify: `tests/player_test.cpp` — add side-entry case tagged `[player][edge]`.

**Interfaces:**
- Consumes: Task 1 `bake_fence_world`, `cylinder_hits_fences`, `collision_body_from_player`
- Produces: `integrate_player` / `integrate_player_surface` use cylinder vs fences; signatures unchanged. Jump path unchanged (it already calls `integrate_player`).

- [ ] **Step 1: Write failing side-entry test** in `tests/player_test.cpp` (same `make_surface_map` helper):

Map 2×2, ground 0, east fence on (0,0) height 0.45. Player `x=1.3`, `y=0`, `z=-0.5`, `half_extent=0.4`, `speed=4`. Move `axis_z=+1` for 60 frames at 1/60s. **Old** center-cross allows z to reach ~0.5. **New:** cylinder would overlap the east segment while z∈[0,1] at x=1.3, so Z steps revert. `REQUIRE(player.z < 0.0f)`.

Keep existing `"Walk into a 0.45 east fence blocks X like a too-high step-up"` (`player.x < 1.0`).

- [ ] **Step 2: Run `[player][edge]` — side-entry FAIL** (`z` not `< 0`).

- [ ] **Step 3: Wire `player.cpp`.** `#include "rat/collision.hpp"`. At start of each integrate that has a query, `const CollisionWorld fences = bake_fence_world(edge_barriers, query)`. Replace `blocked_by_edge_barriers(...)` with `cylinder_hits_fences(collision_body_from_player(player), fences)`. Delete the old helper entirely.

- [ ] **Step 4: `[player][edge]`, `[collision]`, then full `rat_tests`.** Existing jump-over-0.45 / not-over-1.6 in `player_jump_test.cpp` must stay green.

- [ ] **Step 5: Commit** player.cpp + player_test.cpp.

---

### Task 3: Event overlap uses circle; Systems Index

**Files:**
- Modify: `src/engine/src/event_runtime.cpp` — `player_overlaps` body vs event AABB: circle (center `player.x/z`, radius `half_extent`) vs AABB, not square AABB. Height matching unchanged. Tile **action range** (0.65 tiles to center) unchanged.
- Modify: `tests/event_runtime_test.cpp` only if an existing overlap test encodes the square corner; prefer a new `[collision]` or `[event]` case: event volume AABB, player whose **square** corner overlaps but circle does not → `player_overlaps` false; player on the axis within radius → true.
- Modify: `vault/engine/Systems Index.md` — Collision world status `working`; note fence bake + cylinder, not ECS.

**Interfaces:**
- Consumes: none from CollisionWorld required (inline closest-point-on-AABB). May put `circle_overlaps_aabb2` in `collision.hpp` if that avoids duplicating math; if added, test it in `collision_test.cpp`.
- Produces: event PlayerTouch uses circle footprint.

- [ ] **Step 1: Failing test** for circle vs event AABB (corner vs axis).

- [ ] **Step 2: Confirm RED.**

- [ ] **Step 3: Implement circle-AABB overlap** (clamp center to box, dist2 <= r^2).

- [ ] **Step 4: Focused event/collision tests then full `rat_tests`.**

- [ ] **Step 5: Update Systems Index. Commit** event_runtime + tests + wiki. Not the bug status.

---

## Spec coverage

| Spec | Task |
| --- | --- |
| Bake fence quads, owner_top from query | 1 |
| Cylinder radius 0.4 height 1.6 | 1 |
| Jumpable when feet >= top | 1, 2 |
| Replace center-cross in integrate | 2 |
| Side-entry regression | 2 |
| Jump uses same integrate | 2 |
| Events circle XZ | 3 |
| Ground still sample at center | 2 (unchanged) |
| Out of scope cubes/ramps/ECS | all |
| Systems Index | 3 |
