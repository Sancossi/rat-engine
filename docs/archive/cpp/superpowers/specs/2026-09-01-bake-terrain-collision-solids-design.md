# Bake height-grid and ramps to collision solids

Vault: [[feat-bake-height-grid-and-ramps-to-collision-solids|feat: Bake height-grid and ramps to collision solids]] (Sprint 6).
Depends: [[edge-walls-passable-from-adjacent-side]] (shipped).
Not this card: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]], crates, ECS.

## Goal

A Place-cube (+1.0) side is a real wall for the player cylinder: you cannot slide through it from a neighbouring tile. Stairs that already fit `max_step_up` stay walkable. Ramp **low** edge stays the walk-on; ramp **sides** block like greybox trapezoid walls.

## Authoring vs runtime

Edit is unchanged (height-grid, ramps, fences). Runtime bakes:

| Authoring | Bake |
| --- | --- |
| `edge_barriers` | fence segments (already) |
| height-grid + ramps | `build_terrain_side_faces` → the same segment+Y solids as greybox walls |

Standing Y this card: still `SurfaceQuery.sample` at feet center (ramps keep interpolated slope). Dropping sample for support is stacked-surfaces, not this card.

## Solids

Reuse `FenceSolid` (XZ segment + `y_lo`/`y_hi`). For a `TerrainSideFace`, `y_lo = min(y0_lo,y1_lo)`, `y_hi = max(y0_hi,y1_hi)` (conservative trapezoid).

Hit: same as fences (circle vs segment, Y overlap). Additionally:

- If `body.y + 1e-4 >= y_hi`, do not block (standing on the high side).
- If `(y_hi - y_lo) <= max_step_up`, do not block (0.25 stairs). Default `max_step_up` is **0.35**. A +1.0 cube wall is 1.0 > 0.35 → blocks.

## Integrate

`bake_collision_world(const MapData&, const SurfaceQuery&)` = fences + terrain sides.

`integrate_player_surface` / `integrate_player_frame_surface` / airborne `integrate_player` use that world when a `const MapData*` is passed; if null, fences-only (existing tests). Editor always passes the live map.

Do not replace center step-up (`surface_step_allowed`).

## Tests `[collision]` / `[player][surface]`

- Bake a 1×2 grid, tile (1,0) at y=1: east face of (0,0) or west of (1,0) has span 1.0.
- Cylinder on low tile at x overlapping the face, y=0 → hit; y=1 → no hit.
- Stair 0 and 0.25: wall span 0.25 ≤ 0.35 → no hit; existing walk-up still works.
- Side-walk along a 1.0 cube (center never on the high tile) → blocked (new `integrate` test with `MapData*`).
- Existing ramp low-edge and fence tests stay green.

## Out of scope

Filled AABB under every tile, wedge-as-support, stacked floors, ECS, changing 0.35 / 0.45 / 1.6.
