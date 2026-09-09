# Collision world and fence solids

Vault: [[edge-walls-passable-from-adjacent-side]] (Sprint 6).
Follow-ups (not this slice): [[feat: Bake height-grid and ramps to collision solids]],
[[feat: Field physics puzzles]], [[feat: Stacked surfaces caves and basements]].
ECS: do not introduce; [[research: Entity model ECS vs scene vs hybrid]] still gates any ECS code.

## Goal

Stop walking through an edge fence from a neighbouring tile. Movement hits use a
player **cylinder** against **baked fence solids**, not a center-crossing line
test. The same `CollisionWorld` is the seam for later crates, plates, charge-break
doors, and stacked floors.

## Why the current test fails

`blocked_by_edge_barriers` rejects a step only when the player **center** crosses
the edge plane. `half_extent` is used only along the fence length. Walking
parallel on an adjacent tile never crosses the plane, so the AABB can occupy the
wall.

## Authoring vs runtime

Edit keeps height-grid, ramps, and `edge_barriers` as today (convenient map
tools). Runtime **bakes** what this slice needs:

| Authoring | This slice bakes |
| --- | --- |
| `edge_barriers` | static fence solids (vertical quads) |
| height-grid cubes, ramps | unchanged: still `SurfaceQuery` / step-up |
| blockers | unchanged: XZ AABB + `blocker_blocks_feet` |

Later bake of cubes/ramps is a separate card. Multi-level maps are **stacked
grid layers + extra volumes**, not a voxel world.

## Collision types (`rat_core`)

`CollisionWorld` owns:

- **Static solids** — fences this slice. A fence solid is a vertical quad:
  segment in XZ on the owner-tile edge, Y from `owner_top` to `owner_top +
  height`. `owner_top` is `SurfaceQuery.sample` at the tile center (same as
  today). Rebuild on map load and hot-apply / elevation apply (same times
  `SurfaceQuery` is rebuilt).
- **Dynamic bodies** — the player. Struct is a POD: position (`x,y,z`), cylinder
  `radius` / `height`, `mass` (player = 1), `velocity` (xz + y, may be zero this
  slice). No entity IDs, no ECS. Later crates are more bodies in the same list.

Player cylinder (locked):

- Radius **0.4** (diameter 0.8, fits a 1-tile corridor).
- Height **1.6** from feet (`player.y` is feet; top is `player.y + 1.6`).
- XZ is a **circle**, not the old square AABB.

`player.half_extent` stays 0.4 and is the cylinder radius. Greybox may keep the
square footprint for now; collision must use the circle.

## Queries this slice implements

1. **`cylinder_hits_fence(body, world)`** — true if the cylinder overlaps a fence
   quad: circle vs XZ segment, and `[feet, feet+height]` overlaps
   `[owner_top, owner_top+height]`. If `feet + 1e-4 >= owner_top + height`, the
   fence does not block (Mini 0.45 remains jumpable).
2. **`move_axis`** — existing substep X then Z. After each axis, if the cylinder
   hits a fence **or** overlaps a blocking blocker AABB, revert that axis.
   Replace `blocked_by_edge_barriers` with (1). Do not keep the center-cross test.
3. **Jump / airborne** — same fence test at the current feet Y (including
   `jump_offset` already folded into `player.y`). No new jump solver.
4. **Events** — `player_overlaps` / action range that currently build a square
   AABB from `half_extent` use the **circle** in XZ (distance to AABB or
   equivalent). Height matching stays `event_height_matches_player`.

Support / ground this slice: still `SurfaceQuery.sample` at the **feet center**.
Do not encode “one Y per XZ” into `CollisionBody`; a later support query will
take a Y range for stacked floors.

## Explicitly out of scope

- Baking height-grid boxes or ramp wedges.
- Pushable crates, floor plates, collapsing tiles, charge-break doors.
- Stacked surfaces, caves, interiors.
- ECS, physics engine, spatial partition.
- Changing Mini/Full heights (0.45 / 1.6) or fence-on-ramp authoring rules.

## Tests (`[collision]` and existing `[edge]` / `[player]`)

Headless Catch2, no window.

- Walk into an east 0.45 fence from the owner tile still blocks X (existing case).
- **Side entry:** stand on the adjacent tile so the center never crosses the
  edge; walk parallel so the cylinder overlaps the quad → blocked.
- Jump over 0.45 still clears; 1.6 still does not (existing jump cases, now via
  cylinder vs solid).
- `half_extent` / radius 0.4 still fits through a 1.0-wide gap with no fence.

## Integration

`integrate_player` / `integrate_player_surface` /
`integrate_player_frame_surface` take a `CollisionWorld` (or build one from map
+ `SurfaceQuery` at the call site). Editor / probe rebuild the world when the
map or elevation changes.

Update [[Systems Index]] Collision world from `planned` to `stub`/`working` when
this ships.
