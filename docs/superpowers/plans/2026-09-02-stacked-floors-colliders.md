# Stacked Floors Colliders Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Play stands and walks on baked solids (ground boxes, ramp prisms, airborne floor slabs) and climbs ladders with move keys, so a house can have a second floor and a ceiling.

**Architecture:** Schema v3 adds `floor_slabs` and `ladders`. `CollisionWorld` gains walkable volumes; Play support and ceiling come from those solids when `map != nullptr`, not from `SurfaceQuery.sample`. Floor-slab and ladder tools write `EditHistory` like ramps/fences. Greybox draws the new volumes.

**Tech Stack:** C++20, Catch2, `rat_core` (glfw-free). Windows: `VsDevCmd.bat -arch=x64` then cmake from VS BuildTools; existing `build/` Ninja Release.

## Global Constraints

- Spec: `docs/superpowers/specs/2026-09-02-stacked-floors-colliders-design.md`
- Vault: `vault/production/tasks/feat-stacked-surfaces-caves-and-basements.md` — implementer does **not** set Done / In review / Fixed
- Cylinder radius **0.4**, height **1.6**; `max_step_up` default **0.35**; Mini fence **0.45** still blocks at `y≈0.10`
- Do **not** restore global lip-skip in `cylinder_hits_walls`
- Default slab `thickness` **0.25**; ladder inset **0.3** into owner tile
- `SurfaceQuery` may remain for Edit pick / greybox and for Play calls with `map == nullptr`. When `map != nullptr`, standing Y must not use `sample()` as source of truth
- Jump coyote/buffer stay; west-climb / walk-off / Mini / cube-drop depenetration tests with `&map` stay green (implementation may change)
- No parapet-on-fence, no voxel world, no second painted height-grid, no Interact-to-climb, no auto-extend edge barriers to slabs
- `rat_core` stays glfw/miniaudio-free; do not commit `imgui.ini`
- Tests (from repo root). `cmake.exe` is the VS BuildTools copy:

```bat
cmd /c "call \"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat\" -arch=x64 && \"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe\" --build c:\5_gamedev\rat-engine\build --target rat_tests && c:\5_gamedev\rat-engine\build\tests\rat_tests.exe \"[tag]\""
```

## File map

- Modify: `src/engine/include/rat/map_data.hpp` — `FloorSlabDef`, `LadderDef`, `MapData` vectors, `kDefaultFloorSlabThickness`
- Modify: `src/engine/src/map_loader.cpp` — schema 1/2/3 parse/serialize
- Modify: `src/engine/src/map_document.cpp` — accept 3; validate slabs/ladders
- Modify: `docs/schemas/map-event.schema.md` — v3 fields
- Modify: `src/engine/include/rat/collision.hpp`, `src/engine/src/collision.cpp` — walkable volumes, bake, support, ceiling, ladder overlap
- Modify: `src/engine/src/player.cpp`, `src/engine/src/player_jump.cpp` — support from solids; ceiling; ladder climb
- Modify: `src/engine/include/rat/height_edit.hpp`, `src/engine/src/height_edit.cpp` — upsert/toggle slab, upsert/remove ladder, bump schema to 3
- Modify: `src/engine/include/rat/edit_history.hpp`, `src/engine/src/edit_history.cpp` — commands wrapping those helpers
- Modify: `src/engine/include/rat/viewport_edit.hpp`, `src/engine/src/viewport_edit.cpp` — `PlaceSlab` / `PlaceLadder`
- Modify: `apps/editor/panels/terrain_panel.hpp/.cpp`, `apps/editor/editor_app.cpp/.hpp` — panel + click + history
- Modify: `src/engine/src/greybox.cpp` — draw slabs and ladders
- Modify: `tests/map_loader_test.cpp`, `tests/map_document_test.cpp`, `tests/collision_test.cpp`, `tests/player_test.cpp`, `tests/player_jump_test.cpp`, `tests/edit_history_test.cpp`, `tests/viewport_edit_test.cpp`, `tests/height_edit_test.cpp` if helpers are tested there
- Modify: `vault/engine/Systems Index.md` collision row

---

### Task 1: Schema v3 floor slabs and ladders

**Files:**
- Modify: `src/engine/include/rat/map_data.hpp`
- Modify: `src/engine/src/map_loader.cpp`
- Modify: `src/engine/src/map_document.cpp` (`validate_map_document` and compile schema check around the current `expected 1 or 2` strings)
- Modify: `docs/schemas/map-event.schema.md`
- Test: `tests/map_loader_test.cpp`
- Test: `tests/map_document_test.cpp`

**Interfaces:**
- Consumes: existing `TileCoord`, `RampDirection`, local `parse_ramp_direction` / `ramp_direction_to_string` in `map_loader.cpp`
- Produces:

```cpp
inline constexpr float kDefaultFloorSlabThickness = 0.25f;

struct FloorSlabDef {
  TileCoord tile;
  float top_y = 0.0f;
  float thickness = kDefaultFloorSlabThickness;
};

struct LadderDef {
  TileCoord tile;
  RampDirection direction = RampDirection::East;
  float y_lo = 0.0f;
  float y_hi = 1.6f;
};

// on MapData:
std::vector<FloorSlabDef> floor_slabs;
std::vector<LadderDef> ladders;
```

- Loader accepts `schema_version` 1, 2, or **3**. v1/v2 ignore `floor_slabs`/`ladders` keys (empty vectors), same as v1 ignores `edge_barriers`. v3 requires `height_grid` like v2; missing slab/ladder keys = empty.
- Serialize: `if (schema_version >= 2)` write height_grid/ramps/edge_barriers (today this is `== 2` — change it). `if (schema_version >= 3)` write `floor_slabs` and `ladders`.
- Omit `thickness` in JSON → `kDefaultFloorSlabThickness`.
- Validation (error): `thickness <= 0`; slab tile outside height_grid; two slabs on the same tile whose Y ranges `[top_y - thickness, top_y]` overlap (treat as overlap if `a_lo < b_hi - 1e-4f && b_lo < a_hi - 1e-4f`); ladder `y_hi <= y_lo`; ladder tile OOB. Paths: `/floor_slabs/0/thickness`, `/floor_slabs/1/top_y`, `/ladders/0/y_hi`, `/ladders/0/tile`.
- Existing tests that reject schema **3** as unknown must reject **4** instead:
  - `tests/map_loader_test.cpp` `"Map loader rejects unknown schema version"`
  - `tests/map_document_test.cpp` `"schema error from JSON is structured with path"`

- [ ] **Step 1: Write failing tests**

Replace the unknown-schema JSON `schema_version":3` with `4` in both files above (those cases stay passing). Then add:

```cpp
TEST_CASE("Map loader v2 omits floor_slabs and ladders", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 2,
    "id": "v2_empty_slabs",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.floor_slabs.empty());
  REQUIRE(loaded.map.ladders.empty());
}

TEST_CASE("Map loader v3 roundtrips floor slab and ladder", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "house",
    "width": 2,
    "height": 2,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 2, "height": 2,
      "ground_y": [0.0, 0.0, 0.0, 0.0]
    },
    "floor_slabs": [
      { "tile": { "x": 1, "z": 2 }, "top_y": 2.0, "thickness": 0.25 }
    ],
    "ladders": [
      { "tile": { "x": 0, "z": 0 }, "direction": "east", "y_lo": 0.0, "y_hi": 2.0 }
    ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.schema_version == 3);
  REQUIRE(loaded.map.floor_slabs.size() == 1);
  REQUIRE(loaded.map.floor_slabs[0].tile.x == 1);
  REQUIRE(loaded.map.floor_slabs[0].tile.z == 2);
  REQUIRE(loaded.map.floor_slabs[0].top_y == Catch::Approx(2.0f));
  REQUIRE(loaded.map.floor_slabs[0].thickness == Catch::Approx(0.25f));
  REQUIRE(loaded.map.ladders.size() == 1);
  REQUIRE(loaded.map.ladders[0].direction == rat::RampDirection::East);
  REQUIRE(loaded.map.ladders[0].y_hi == Catch::Approx(2.0f));

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.floor_slabs.size() == 1);
  REQUIRE(again.map.ladders[0].tile.x == 0);
}

TEST_CASE("v3 slab missing thickness defaults to 0.25", "[unit][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 3,
    "id": "thin",
    "width": 1, "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "floor_slabs": [ { "tile": { "x": 0, "z": 0 }, "top_y": 1.6 } ],
    "events": []
  })";
  const auto loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.floor_slabs[0].thickness == Catch::Approx(rat::kDefaultFloorSlabThickness));
}
```

In `map_document_test.cpp`:

```cpp
TEST_CASE("two stacked slabs on one tile are valid; overlapping Y is an error", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.floor_slabs.push_back({{0, 0}, 4.0f, 0.25f});
  REQUIRE_FALSE(rat::map_issues_have_errors(rat::validate_map_document(map)));

  map.floor_slabs[1].top_y = 2.1f;
  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/floor_slabs/1/top_y"));
}

TEST_CASE("ladder with y_hi <= y_lo reports /ladders/0/y_hi", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 0.0f});
  REQUIRE(has_error_at(rat::validate_map_document(map), "/ladders/0/y_hi"));
}
```

- [ ] **Step 2: Run tests — RED**

Run: the bat command with `"[unit][map]"` then `"[unit][mapdoc]"`.

Expected: `Map loader v3 roundtrips...` FAIL (`unsupported schema_version` or `floor_slabs` is not a member of `MapData`). Overlap test FAIL (no validation path).

- [ ] **Step 3: Implement**

Add the structs to `map_data.hpp`. In `parse_map`, accept 1/2/3; for v3 parse `floor_slabs`/`ladders` arrays (reuse ramp direction parser). For v2 keep requiring `height_grid`. In `serialize_map_to_string`, write elevation when `>= 2` and slabs/ladders when `>= 3`. In `validate_map_document` / compile, expected versions `1, 2, or 3`, plus the overlap/OOB/thickness/`y_hi` checks. Document v3 in `docs/schemas/map-event.schema.md`.

- [ ] **Step 4: GREEN**

Run `[unit][map]` and `[unit][mapdoc]`. Then full `ctest` from `build/` in case other schema-3 fixtures exist (there should not be after Step 1).

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/map_data.hpp src/engine/src/map_loader.cpp src/engine/src/map_document.cpp docs/schemas/map-event.schema.md tests/map_loader_test.cpp tests/map_document_test.cpp
git commit -m "Add schema v3 floor slabs and ladders so maps can author airborne floors."
```

---

### Task 2: Bake walkable boxes (ground + slabs)

**Files:**
- Modify: `src/engine/include/rat/collision.hpp`
- Modify: `src/engine/src/collision.cpp`
- Test: `tests/collision_test.cpp`

**Interfaces:**
- Consumes: Task 1 `FloorSlabDef`; existing `bake_collision_world`, `HeightGrid`, `RampDef`
- Produces:

```cpp
struct WalkableBox {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
  float y_lo = 0.0f;
  float y_hi = 0.0f;  // standable top; underside is ceiling
};

struct CollisionWorld {
  std::vector<FenceSolid> fences;
  std::vector<WalkableBox> boxes;
};

void append_ground_boxes(CollisionWorld& world, const HeightGrid& grid,
                         std::span<const RampDef> ramps, float tile_size);
void append_floor_slabs(CollisionWorld& world, std::span<const FloorSlabDef> slabs, float tile_size);
```

- `append_ground_boxes`: for each cell whose `(origin+x, origin+z)` is **not** a ramp tile, one box: XZ = cell world AABB, `y_lo = 0`, `y_hi = ground_y` (including `ground_y == 0`). These boxes are **support only** — do not emit extra `FenceSolid`s for ground boxes (terrain side faces already cover stairs; a 0.25 stair must still be skipped by `max_step_up`).
- `append_floor_slabs`: XZ = cell, `y_hi = top_y`, `y_lo = top_y - thickness`. Do **not** fill down to 0.
- `bake_collision_world`: keep fences + `append_terrain_walls`; also call both appends.
- Helper (can be file-local until Task 4): circle vs box XZ using existing `circle_overlaps_aabb2` with `Aabb2{min_x,min_z,max_x,max_z}`.

- [ ] **Step 1: Failing tests** in `collision_test.cpp` (reuse `make_grid`):

```cpp
TEST_CASE("Bake ground box for a 1.0 cell and skip ramp tiles", "[collision]") {
  rat::MapData map = make_grid(1, 1, 1.0f);
  const rat::SurfaceQuery query(map);
  rat::CollisionWorld world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.size() == 1);
  REQUIRE(world.boxes[0].y_hi == Approx(1.0f));
  REQUIRE(world.boxes[0].y_lo == Approx(0.0f));

  map.ramps.push_back({.tile = {0, 0}, .direction = rat::RampDirection::East, .low_y = 0.0f, .high_y = 1.0f});
  world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.empty());  // prism comes in Task 3
}

TEST_CASE("Bake floor slab is a thin box not filled to Y=0", "[collision]") {
  rat::MapData map = make_grid(1, 1, 0.0f);
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.size() == 2);
  bool found_slab = false;
  for (const rat::WalkableBox& box : world.boxes) {
    if (box.y_hi == Approx(2.0f) && box.y_lo == Approx(1.75f)) {
      found_slab = true;
    }
  }
  REQUIRE(found_slab);
}
```

- [ ] **Step 2: RED** — `boxes` is not a member of `CollisionWorld`.

- [ ] **Step 3: Implement** the structs, append functions, and wire `bake_collision_world`. Existing fence/side-face tests must still pass (they do not inspect `boxes`).

- [ ] **Step 4: `[collision]` GREEN.**

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/collision.hpp src/engine/src/collision.cpp tests/collision_test.cpp
git commit -m "Bake ground and floor-slab boxes so airborne floors do not fill down to Y=0."
```

---

### Task 3: Ramp prisms as walkable tops

**Files:**
- Modify: `src/engine/include/rat/collision.hpp`
- Modify: `src/engine/src/collision.cpp`
- Test: `tests/collision_test.cpp`

**Interfaces:**
- Consumes: `RampDef`, `RampDirection`
- Produces:

```cpp
struct WalkableRamp {
  TileCoord tile;
  RampDirection direction = RampDirection::East;
  float low_y = 0.0f;
  float high_y = 0.0f;
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
};

// add to CollisionWorld:
std::vector<WalkableRamp> ramps;

[[nodiscard]] float ramp_surface_y(const WalkableRamp& ramp, float x, float z);
void append_ramp_prisms(CollisionWorld& world, std::span<const RampDef> ramps, float tile_size);
```

Copy interpolation from `SurfaceQuery::sample` in `src/engine/src/surface_query.cpp` (do not call `SurfaceQuery` from this helper):

```text
frac_x = clamp01((x - min_x) / (max_x - min_x))
frac_z = clamp01((z - min_z) / (max_z - min_z))
North: t = 1 - frac_z
East:  t = frac_x
South: t = frac_z
West:  t = 1 - frac_x
y = low_y + (high_y - low_y) * t
```

`bake_collision_world` calls `append_ramp_prisms`. Sides stay `append_terrain_walls`.

- [ ] **Step 1: Failing test**

```cpp
TEST_CASE("West ramp prism interpolates like SurfaceQuery and replaces the ground box",
          "[collision]") {
  rat::MapData map = make_grid(1, 1, 0.0f);
  map.ramps.push_back({.tile = {0, 0},
                       .direction = rat::RampDirection::West,
                       .low_y = 0.0f,
                       .high_y = 1.0f});
  const rat::SurfaceQuery query(map);
  const rat::CollisionWorld world = rat::bake_collision_world(map, query);
  REQUIRE(world.boxes.empty());
  REQUIRE(world.ramps.size() == 1);
  REQUIRE(rat::ramp_surface_y(world.ramps[0], 0.0f, 0.5f) == Approx(query.sample(0.0f, 0.5f).y).margin(1e-4f));
  REQUIRE(rat::ramp_surface_y(world.ramps[0], 1.0f, 0.5f) == Approx(query.sample(1.0f, 0.5f).y).margin(1e-4f));
}
```

West: `t = 1 - frac_x`, so x=0 → t=1 → y=1 (high), x=1 → t=0 → y=0 (low). Assert against `query.sample`, not a guessed edge.

- [ ] **Step 2: RED.**

- [ ] **Step 3: Implement** interpolation + `append_ramp_prisms` + bake.

- [ ] **Step 4: `[collision]` GREEN.**

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/collision.hpp src/engine/src/collision.cpp tests/collision_test.cpp
git commit -m "Bake ramp tops as prisms so slope support does not need SurfaceQuery."
```

---

### Task 4: Support and ceiling from solids (Play)

**Files:**
- Modify: `src/engine/include/rat/collision.hpp`
- Modify: `src/engine/src/collision.cpp`
- Modify: `src/engine/src/player.cpp`
- Modify: `src/engine/src/player_jump.cpp`
- Test: `tests/collision_test.cpp`, `tests/player_test.cpp`, `tests/player_jump_test.cpp`

**Interfaces:**
- Consumes: `WalkableBox`, `WalkableRamp`, existing jumpable-blocker support (keep it)
- Produces:

```cpp
struct SolidSupport {
  float y = 0.0f;
  bool on_ramp = false;
};

[[nodiscard]] std::optional<SolidSupport> query_solid_support(
    const CollisionWorld& world, float x, float z, float radius, float feet_y,
    float max_step_up);

[[nodiscard]] bool cylinder_hits_ceiling(const CollisionBody& body, const CollisionWorld& world);
```

- `query_solid_support`: among boxes whose XZ overlaps the circle (`circle_overlaps_aabb2`), a candidate top is `y_hi` if `y_hi <= feet_y + max_step_up` and the cylinder is not entirely below the underside in a way that would mean "under the slab" (if `feet_y + 1e-4f < y_lo`, that box is not standable — you are under it). Among ramps whose tile AABB overlaps the circle, candidate is `ramp_surface_y`. Pick the **highest** candidate that is `<= feet_y + max_step_up`. Landing from above: if `feet_y >= candidate` (falling onto a top), still accept when `feet_y` is descending through the top (same 1e-4 epsilon as blocker support in `player_jump.cpp`).
- `cylinder_hits_ceiling`: true if XZ overlaps a box and the head interval `[body.y, body.y + body.height]` crosses `y_lo` from below: `body.y < box.y_lo` and `body.y + body.height > box.y_lo` (and feet are below the top, so you are not standing on it).
- **Slab sides:** for each floor-slab box only, append four `FenceSolid`s (the XZ rectangle edges) with that box's `y_lo`/`y_hi`. Add `bool apply_max_step_up_skip = true` on `FenceSolid` (default true so existing terrain/Mini fences unchanged). Slab sides set it **false**: a 0.25-thick floor is a wall, not a stair. Still skip a slab side when `body.y + kFenceFeetClearanceEpsilon >= y_hi` (walking on the top). `cylinder_hits_walls` honors the flag: if `apply_max_step_up_skip && (y_hi - y_lo) <= max_step_up`, skip.
- Play: in `integrate_player_surface` / `integrate_player_frame_surface`, when `map != nullptr`, bake `CollisionWorld` and set standing Y from `query_solid_support` instead of `surface_query.sample`. When `map == nullptr`, keep today's SurfaceQuery path. Ceiling: reject upward jump/motion that would `cylinder_hits_ceiling`.
- Keep jumpable-blocker support. Keep `depenetrate_cylinder_from_walls` on grounded off-ramp landing.

- [ ] **Step 1: Failing tests**

`[collision]` — ceiling and slab-side:

```cpp
TEST_CASE("Cylinder head hits slab underside; feet on top do not", "[collision]") {
  rat::MapData map = make_grid(1, 1, 0.0f);
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::CollisionWorld world = rat::bake_collision_world(map, rat::SurfaceQuery(map));
  rat::CollisionBody under;
  under.x = 0.5f; under.y = 0.0f; under.z = 0.5f;
  under.height = 1.6f;
  REQUIRE(rat::cylinder_hits_ceiling(under, world));
  rat::CollisionBody on_top = under;
  on_top.y = 2.0f;
  REQUIRE_FALSE(rat::cylinder_hits_ceiling(on_top, world));
}

TEST_CASE("Thin slab side blocks at mid-thickness even though span < max_step_up", "[collision]") {
  rat::MapData map = make_grid(2, 1, 0.0f);
  map.floor_slabs.push_back({{1, 0}, 2.0f, 0.25f});
  const rat::CollisionWorld world = rat::bake_collision_world(map, rat::SurfaceQuery(map));
  rat::CollisionBody body;
  body.x = 0.7f; body.y = 1.8f; body.z = 0.5f;
  REQUIRE(rat::cylinder_hits_walls(body, world, 0.35f));
}
```

`[unit][player][surface]` — stand / walk-off (pass `&map`):

```cpp
TEST_CASE("Player stands on airborne slab and falls walking off", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(2, 1, {0.0f, 0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody player;
  player.x = 0.5f; player.y = 2.0f; player.z = 0.5f; player.speed = 5.0f;
  player = rat::integrate_player_surface(player, {}, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  REQUIRE(player.y == Approx(2.0f).margin(1e-3f));
  rat::MoveInput east{1.0f, 0.0f};
  for (int i = 0; i < 40; ++i) {
    player = rat::integrate_player_surface(player, east, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  }
  REQUIRE(player.x > 1.2f);
  REQUIRE(player.y == Approx(0.0f).margin(0.05f));
}
```

`[unit][player][jump]` — hop under slab does not pass through `y_lo=1.75`:

```cpp
TEST_CASE("Jump under a slab does not rise through the underside", "[unit][player][jump]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody body;
  body.x = 0.5f; body.y = 0.0f; body.z = 0.5f;
  rat::JumpState jump = rat::make_grounded_jump_state();
  rat::PlayerFrameInput input;
  input.jump_pressed = true;
  input.jump_held = true;
  for (int i = 0; i < 90; ++i) {
    const auto result = rat::integrate_player_frame_surface(
        body, jump, input, 1.0f / 60.0f, {}, query, {}, 0.35f, {}, &map);
    body = result.body;
    jump = result.jump;
    input.jump_pressed = false;
    REQUIRE(body.y + 1.6f <= 1.75f + 1e-3f);
  }
}
```

Do not rewrite existing west-climb / walk-off / Mini 0.45 / cube-drop tests; they must stay green with `&map`.

- [ ] **Step 2: RED** on slab stand/fall/ceiling.

- [ ] **Step 3: Minimal Play path** — support + ceiling + slab side segments + `apply_max_step_up_skip`. In the `map != nullptr` branch, do not use `surface_query.sample()` for the standing Y.

- [ ] **Step 4: GREEN** `[collision]`, `[unit][player]`, `[unit][player][jump]`, then full `ctest`.

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/collision.hpp src/engine/src/collision.cpp src/engine/src/player.cpp src/engine/src/player_jump.cpp tests/collision_test.cpp tests/player_test.cpp tests/player_jump_test.cpp
git commit -m "Stand and fall using baked solid tops so Play no longer samples SurfaceQuery for Y."
```

---

### Task 5: Ladder overlap and climb

**Files:**
- Modify: `src/engine/include/rat/collision.hpp`
- Modify: `src/engine/src/collision.cpp`
- Modify: `src/engine/src/player_jump.cpp` (climb lives in `integrate_player_frame_surface` so leaving a ladder can fall; also handle grounded-only `integrate_player_surface` the same way so Edit-play without jump still climbs)
- Test: `tests/player_test.cpp` and/or `tests/player_jump_test.cpp`

**Interfaces:**
- Consumes: `LadderDef`, `MoveInput`, cylinder
- Produces:

```cpp
inline constexpr float kLadderInset = 0.3f;

struct LadderVolume {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
  float y_lo = 0.0f;
  float y_hi = 0.0f;
  RampDirection face = RampDirection::East;
};

// add to CollisionWorld:
std::vector<LadderVolume> ladders;

void append_ladders(CollisionWorld& world, std::span<const LadderDef> ladders, float tile_size);
[[nodiscard]] const LadderVolume* overlapping_ladder(const CollisionBody& body,
                                                     const CollisionWorld& world);
```

- Volume: along the named **owner-tile edge** (same edge as `EdgeBarrierDef` / `bake_fence_world`), thickness `kLadderInset` **into** the tile, Y `[y_lo, y_hi]`. Example: tile (0,0) size 1, East → `min_x=0.7`, `max_x=1.0`, `min_z=0`, `max_z=1`. Not a blocking wall.
- Overlap: XZ circle vs volume AABB and body Y overlapping `[y_lo, y_hi]` (allow a small epsilon).
- Climb: if overlapping, map move **into** the face to +Y and **away** to −Y at `player.speed * dt`. Into = toward the named edge. East face: `axis_x > 0` → +Y, `axis_x < 0` → −Y. North: `axis_z < 0` → +Y (toward min_z). Clamp Y to `[y_lo, y_hi]` while overlapping. Do not require `jump_pressed`. Zero the unused XZ walk into the wall; keep the player overlapping the volume.
- Leave volume: `query_solid_support`; if none, airborne (gravity). Ladder is not a walkable floor by itself.

- [ ] **Step 1: Failing test**

```cpp
TEST_CASE("East ladder climb reaches slab with move only", "[unit][player][surface]") {
  rat::MapData map = make_surface_map(1, 1, {0.0f});
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 2.0f});
  const rat::SurfaceQuery query(map);
  rat::PlayerBody player;
  player.x = 0.85f; player.y = 0.0f; player.z = 0.5f; player.speed = 5.0f;
  rat::MoveInput into{1.0f, 0.0f};
  for (int i = 0; i < 80; ++i) {
    player = rat::integrate_player_surface(player, into, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  }
  REQUIRE(player.y == Approx(2.0f).margin(0.05f));
  rat::MoveInput away{-1.0f, 0.0f};
  for (int i = 0; i < 80; ++i) {
    player = rat::integrate_player_surface(player, away, 1.0f / 60.0f, {}, query, 0.35f, {}, &map);
  }
  REQUIRE(player.y == Approx(0.0f).margin(0.1f));
}
```

- [ ] **Step 2: RED.**

- [ ] **Step 3: Implement** volumes, `append_ladders` in bake, climb branch **before** normal XZ walk when overlapping.

- [ ] **Step 4: Ladder test GREEN.** West-climb / Mini still green.

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/collision.hpp src/engine/src/collision.cpp src/engine/src/player.cpp src/engine/src/player_jump.cpp tests/player_test.cpp tests/player_jump_test.cpp
git commit -m "Climb ladders with move keys so a fire escape can reach an airborne slab."
```

---

### Task 6: EditHistory + Floor slab tool + greybox

**Files:**
- Modify: `src/engine/include/rat/height_edit.hpp`, `src/engine/src/height_edit.cpp`
- Modify: `src/engine/include/rat/edit_history.hpp`, `src/engine/src/edit_history.cpp`
- Modify: `src/engine/include/rat/viewport_edit.hpp`, `src/engine/src/viewport_edit.cpp`
- Modify: `apps/editor/panels/terrain_panel.hpp/.cpp`
- Modify: `apps/editor/editor_app.cpp`, `apps/editor/editor_app.hpp` if tool/panel state lives there
- Modify: `src/engine/src/greybox.cpp` (`GreyboxScene::set_terrain_map`)
- Test: `tests/edit_history_test.cpp`, `tests/viewport_edit_test.cpp`, `tests/height_edit_test.cpp`

**Interfaces:**
- Consumes: `FloorSlabDef`, `kDefaultFloorSlabThickness`
- Produces:

```cpp
// height_edit.hpp
HeightEditResult upsert_map_floor_slab(MapData& map, FloorSlabDef slab);  // toggle same (tile, top_y)
// bumps schema_version to 3 (and to 2 first if needed, via upgrade_map_schema_for_elevation)

// edit_history.hpp
std::unique_ptr<EditCommand> make_upsert_map_floor_slab_command(FloorSlabDef slab);
```

- Toggle: if a slab exists on the same tile with `|top_y - slab.top_y| < 1e-4`, remove it; else upsert (replace thickness if same top, or add). `mutates_elevation() == true`. Play must not push the stack (same gating as Place cube in `editor_app.cpp`).
- `ViewportTool::PlaceSlab` and `ViewportClickActionKind::PlaceSlab`; `resolve_viewport_click` returns the hit tile like PlaceCube.
- Panel: `top_y` presets 1.6 / 2.0 / custom float; `thickness` default 0.25. Store on `TerrainPanelState`.
- Greybox: for each slab, draw AABB (top + underside + sides) with a color distinct from terrain grey (`0xff707070`). Extend `terrain_fill_quad_count_fits_u16` (or the local quad budget) so extra quads do not overflow `uint16`; on overflow keep the same fail-safe as today (clear terrain geometry).

- [ ] **Step 1: Failing tests**

```cpp
TEST_CASE("floor slab upsert toggles same top_y and undoes", "[unit][edit][height]") {
  rat::MapData map = make_tiny_map();
  REQUIRE(rat::upgrade_map_schema_for_elevation(map).ok);
  rat::EditHistory history;
  rat::FloorSlabDef slab{{0, 0}, 2.0f, 0.25f};
  REQUIRE(history.execute(map, rat::make_upsert_map_floor_slab_command(slab)));
  REQUIRE(map.schema_version == 3);
  REQUIRE(map.floor_slabs.size() == 1);
  REQUIRE(history.execute(map, rat::make_upsert_map_floor_slab_command(slab)));
  REQUIRE(map.floor_slabs.empty());
  REQUIRE(history.undo(map));
  REQUIRE(map.floor_slabs.size() == 1);
  REQUIRE(history.redo(map));
  REQUIRE(map.floor_slabs.empty());
}

TEST_CASE("place slab on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceSlab, rat::Vec3{2.9f, 0.0f, -0.1f});
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceSlab);
  REQUIRE(action.tile.x == 2);
  REQUIRE(action.tile.z == -1);
}
```

- [ ] **Step 2: RED.**

- [ ] **Step 3: Command + `upsert_map_floor_slab` + viewport tool + panel radio "Floor slab" + editor click in Edit only + greybox quads.**

- [ ] **Step 4: `[unit][edit]` and `[unit][viewport_edit]` GREEN.** `cmake --build ... --target rat-editor` links.

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/height_edit.hpp src/engine/src/height_edit.cpp src/engine/include/rat/edit_history.hpp src/engine/src/edit_history.cpp src/engine/include/rat/viewport_edit.hpp src/engine/src/viewport_edit.cpp apps/editor/panels/terrain_panel.hpp apps/editor/panels/terrain_panel.cpp apps/editor/editor_app.cpp apps/editor/editor_app.hpp src/engine/src/greybox.cpp tests/edit_history_test.cpp tests/viewport_edit_test.cpp tests/height_edit_test.cpp
git commit -m "Add Floor slab edit tool with undo so airborne floors can be authored."
```

---

### Task 7: Ladder edit tool + greybox

**Files:**
- Modify: `src/engine/include/rat/height_edit.hpp`, `src/engine/src/height_edit.cpp`
- Modify: `src/engine/include/rat/edit_history.hpp`, `src/engine/src/edit_history.cpp`
- Modify: `src/engine/include/rat/viewport_edit.hpp`, `src/engine/src/viewport_edit.cpp`
- Modify: `apps/editor/panels/terrain_panel.hpp/.cpp`, `apps/editor/editor_app.cpp`
- Modify: `src/engine/src/greybox.cpp`
- Test: `tests/edit_history_test.cpp`, `tests/viewport_edit_test.cpp`

**Interfaces:**
- Produces:

```cpp
HeightEditResult upsert_map_ladder(MapData& map, LadderDef ladder);  // last-wins (tile, direction)
HeightEditResult remove_map_ladder(MapData& map, TileCoord tile, RampDirection direction);

std::unique_ptr<EditCommand> make_upsert_map_ladder_command(LadderDef ladder);
std::unique_ptr<EditCommand> make_remove_map_ladder_command(TileCoord tile, RampDirection direction);
```

- Last-wins on `(tile, direction)`. Bump schema to 3. `mutates_elevation() == true`.
- Panel: `y_lo`, `y_hi` (default 0 … 1.6). If a selected/hovered slab exists, default `y_hi` to that slab's `top_y`.
- `ViewportTool::PlaceLadder`; click uses current edge direction from `TerrainPanelState.edge_direction_index` (same N/E/S/W as Fence).
- Greybox: thin volume, color distinct from Mini/Full fence (`0xff8a5a38`). Same uint16 fail-safe.

- [ ] **Step 1: Failing tests**

```cpp
TEST_CASE("ladder upsert last-wins on tile and direction and undoes", "[unit][edit][height]") {
  rat::MapData map = make_tiny_map();
  REQUIRE(rat::upgrade_map_schema_for_elevation(map).ok);
  rat::EditHistory history;
  rat::LadderDef a{{0, 0}, rat::RampDirection::East, 0.0f, 1.6f};
  rat::LadderDef b = a;
  b.y_hi = 2.0f;
  REQUIRE(history.execute(map, rat::make_upsert_map_ladder_command(a)));
  REQUIRE(history.execute(map, rat::make_upsert_map_ladder_command(b)));
  REQUIRE(map.ladders.size() == 1);
  REQUIRE(map.ladders[0].y_hi == Approx(2.0f));
  REQUIRE(history.undo(map));
  REQUIRE(map.ladders[0].y_hi == Approx(1.6f));
}

TEST_CASE("place ladder on empty tile returns that tile", "[unit][viewport_edit]") {
  rat::MapData map = make_test_map();
  const rat::ViewportClickAction action =
      rat::resolve_viewport_click(map, rat::ViewportTool::PlaceLadder, rat::Vec3{4.1f, 0.0f, 3.7f});
  REQUIRE(action.kind == rat::ViewportClickActionKind::PlaceLadder);
  REQUIRE(action.tile.x == 4);
  REQUIRE(action.tile.z == 3);
}
```

- [ ] **Step 2: RED.**

- [ ] **Step 3: Implement** commands + tool + greybox + editor click in Edit only.

- [ ] **Step 4: Tests GREEN.** Full `ctest`. `rat-editor` links.

- [ ] **Step 5: Commit**

```bash
git add src/engine/include/rat/height_edit.hpp src/engine/src/height_edit.cpp src/engine/include/rat/edit_history.hpp src/engine/src/edit_history.cpp src/engine/include/rat/viewport_edit.hpp src/engine/src/viewport_edit.cpp apps/editor/panels/terrain_panel.hpp apps/editor/panels/terrain_panel.cpp apps/editor/editor_app.cpp src/engine/src/greybox.cpp tests/edit_history_test.cpp tests/viewport_edit_test.cpp
git commit -m "Add ladder edit tool so fire-escape volumes can be placed on tile edges."
```

---

### Task 8: Systems Index

**Files:**
- Modify: `vault/engine/Systems Index.md` — Collision row: walkable boxes, ramp prisms, floor slabs, ladders; Play support from solids when map is passed. Keep the stacked-surfaces wikilink.

Do not add a grey_yard demo slab (smoke maps stay schema 2). Do not flip the vault task to Done.

- [ ] **Step 1:** Update the Collision world note to match shipped API after Tasks 1–7.
- [ ] **Step 2:** Full `ctest` still 100% (wiki-only).
- [ ] **Step 3: Commit**

```bash
git add "vault/engine/Systems Index.md"
git commit -m "Document collider floors and ladders on the systems index."
```

---

## Spec coverage (self-review)

| Spec item | Task |
| --- | --- |
| schema v3 slabs/ladders, default thickness 0.25, overlap validation | 1 |
| v1/v2 empty lists; unknown version becomes 4 in tests | 1 |
| bake ground box, slab AABB, no fill under slab | 2 |
| ramp prism instead of flat box; interpolation matches SurfaceQuery | 3 |
| Play support/ceiling/fall from solids when `map != nullptr` | 4 |
| Mini 0.45, no lip-skip, west-climb/walk-off tests | 4 |
| thin slab sides are walls (not max_step_up stairs) | 4 |
| ladder volume, move into = +Y, leave → stand or fall | 5 |
| Floor slab tool, same top_y toggle, undo, greybox | 6 |
| Ladder tool, last-wins, undo, greybox | 7 |
| Systems Index | 8 |
| Out of scope parapet / voxels / second grid / Interact-to-climb | Global Constraints |

## Type consistency

- `CollisionWorld`: `fences`, `boxes` (Task 2), `ramps` (Task 3), `ladders` (Task 5)
- `FloorSlabDef` / `LadderDef` live on `MapData`; bake copies them into `WalkableBox` / `LadderVolume`
- `kDefaultFloorSlabThickness = 0.25f`, `kLadderInset = 0.3f`
- Fence side skip: `FenceSolid::apply_max_step_up_skip` (default true)
