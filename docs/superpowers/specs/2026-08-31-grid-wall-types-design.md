# Grid wall types

## Goal

Authors can place a standable terrain cube on a height-grid cell and a fence on a cell edge. Walking across that edge is blocked unless the player's feet clear the fence top. Mini vs full is authored height versus jump apex, not two types.

## Confirmed decisions

- Terrain wall is a standable cube: sides block when the rise exceeds `max_step_up` (0.35); the top is standable.
- One edge primitive with `height` above the owner cell top. Jump apex at current tuning is about `7.5² / (2 * 24) ≈ 1.17`. Edit presets: mini `0.45`, full `1.6`.
- Edges attach to a terrain cell (N/E/S/W of its top), not a prop AABB.
- Edge barriers are not jumpable-blocker supports: the player cannot stand on the fence.
- Stay on schema v2 (optional `edge_barriers` array, like `ramps`). No schema v3.
- Not Sprint 4. Undo for height/edges and mouse viewport place are out of scope.

## Terrain cube

Raised `ground_y` already is the collision cube. This feature adds:

- Vertical side quads wherever a cell is strictly higher than its neighbor (missing neighbor treated as `y = 0`).
- `place_map_terrain_wall_cube`: add `kTerrainWallCubeHeight` (`1.0`) to the cell. Reject ramp tiles.

Jumpable `BlockerDef` slabs stay as props. They are not this primitive.

## Edge barrier

```json
"edge_barriers": [
  { "tile": { "x": 3, "z": 5 }, "direction": "north", "height": 0.45 }
]
```

- `height > 0`. World top is `ground_y(owner) + height`.
- Canonicalize opposites: north of `(x, z)` is south of `(x, z - 1)`; west of `(x, z)` is east of `(x - 1, z)`. Prefer east/south of the in-grid owner. Duplicate canonical edges keep the greater height.
- Reject (or skip) barriers whose owner tile has a ramp.

### Collision

In the existing X-then-Z substeps of `integrate_player`, `integrate_player_surface`, and airborne `integrate_player_frame_surface`:

- If the player AABB straddles the edge segment and `feet_world_y < barrier_top_y - eps`, revert that axis.
- If feet are at or above the top, the step is allowed (cleared jump).
- Walking parallel is allowed until the AABB hits the line.

`half_extent` 0.4 and tile size 1.0: a player at the cell center does not touch edges.

Pass an `EdgeBarrierQuery { barriers, height_grid, tile_size }` into integrate (default empty so existing callers stay valid).

## Rendering and Edit

- Greybox: terrain side quads plus a thin vertical quad per edge barrier.
- Edit: "Place wall cube (+1)" on the selected tile; N/E/S/W + mini/full/custom height to add or remove an edge.
- Sample `grey_yard` with one cube, one mini edge, one full edge.

## Acceptance tests

- Side faces exist between a 1.0 cell and a 0.0 neighbor.
- Place cube raises `ground_y` by 1.0 and refuses ramps.
- Walking into an edge at matching height is blocked; a 0.45 edge is cleared by a full jump; a 1.6 edge is not.
- Edge is never recorded as `support_blocker_index`.
- Loader roundtrip; opposite-direction duplicates collapse; ramp owner rejected.
- Ramp and jumpable-blocker regressions still pass.

## Out of scope

- Fences on prop AABB faces.
- Climbing, breakable walls.
- Schema v3, undo for height/edges, mouse place in the viewport.
