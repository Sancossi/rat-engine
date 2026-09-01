# Stacked floors: collider locomotion, slabs, ladders

Vault: [[feat: Stacked surfaces caves and basements]].
Depends: [[feat: Bake height-grid and ramps to collision solids]] (shipped — sides only).
Chat 2026-09-02: multi-level house (airborne floor = ceiling below), Floor slab tool, collider support including **ramp tops**, climbable ladders.

Not this card: wall-top parapet, voxel world, second height-grid paint layer, archetype ECS, Interact-to-climb.

## Goal

Play standing and walking come from **baked solids**, not `SurfaceQuery.sample`. A cell can have ground, a ramp prism, and one or more airborne **floor slabs**. Slab top is a floor; slab bottom is a ceiling. A **ladder** on an edge climbs with the same move keys (no jump, no Interact).

## Schema (v3)

`schema_version` 3. v1/v2 load with empty `floor_slabs` and `ladders`.

```text
floor_slabs[]: { tile: {x,z}, top_y, thickness }
ladders[]:     { tile: {x,z}, direction: N|E|S|W, y_lo, y_hi }
```

- `thickness` default **0.25** (authorable; 1.0 allowed). Bottom = `top_y - thickness`. Reject `thickness <= 0` and `top_y` overlapping another slab on the same tile (Y ranges intersect).
- Same `(tile, top_y)` upsert: second Floor-slab click removes the slab (toggle).
- Same `(tile, direction)` ladder last-wins; `y_hi > y_lo`; drop if OOB / non-positive span.
- `height_grid`, `ramps`, `edge_barriers` unchanged as authoring.

## Bake

`CollisionWorld` gains walkable volumes (boxes / ramp prisms), not only `FenceSolid` sides.

| Authoring | Solid |
| --- | --- |
| height-grid cell (no ramp) | box XZ = tile, Y = `[0, ground_y]` (flat walkable top at `ground_y`; `ground_y == 0` still a top at 0) |
| ramp | prism on that tile **instead of** the flat top box: slope `low_y` → `high_y`; sides stay as today’s terrain walls |
| floor slab | AABB: top `top_y`, bottom `top_y - thickness`, XZ = tile. Top = support, bottom = ceiling, sides = walls |
| edge barrier | existing fence segment |
| ladder | thin volume along the edge, Y `[y_lo, y_hi]`, ~0.3 into the owner tile (overlap, not a blocking wall) |

Do not fill the column under a slab down to Y=0.

## Locomotion (Play)

Cylinder vs this world:

- **Support:** highest solid **top** under the footprint whose Y is reachable (stand / `max_step_up` / landing), same idea as jumpable-blocker support. Ramp top is the sloped face, not a flat sample.
- **Walls:** existing `cylinder_hits_walls` + slab/box sides. Mini 0.45 still hits when feet are not over `y_hi`. Do **not** restore global lip-skip.
- **Ceiling:** cylinder head vs slab bottoms.
- **Fall:** no support under feet → gravity. Missing slab = hole.
- **`SurfaceQuery`:** not the Play source of truth. May remain for Edit pick / greybox until those read the same bake.

Jump coyote/buffer stay. West-climb, ramp walk-off, cube drop depenetration keep their tests; implementation may change.

### Ladders

While the cylinder overlaps a ladder volume:

- Move **into** the ladder face → +Y; **away** → −Y (same WASD / `InputFrame.move`). Jump not required. Interact not used.
- Leaving the volume: if a floor top is under feet, stand; else fall.
- Ladder is not a walkable floor by itself.

## Editor

Tool **Floor slab** next to Place cube / Fence:

- Panel: `top_y` presets 1.6 / 2.0 / custom; `thickness` (default 0.25).
- Click tile: upsert/toggle slab at that `top_y`.
- Edit writes `EditHistory`; Play does not.
- Greybox: box with visible underside.

Tool **Ladder** (or Fence-mode sibling): click edge + `y_lo` / `y_hi` (defaults 0 … selected slab `top_y` or 1.6). Greybox: thin climb volume.

## Tests (`[collision]` / `[player]` / `[editordoc]`)

- Slab: stand on `top_y`; from below, head hits bottom; walk off → fall to ground top.
- Two slabs, different `top_y`, same tile: both valid; overlapping Y → validation error.
- Ramp: west-climb and walk-off without Play calling `SurfaceQuery.sample` for standing Y.
- Mini 0.45 still blocks at `y≈0.10`.
- Ladder: ground → slab and back using move only; overlap required.

## Out of scope

Parapet on fence tops, voxel sculpt, second painted height layer, OpenAL, ECS, auto-extending `edge_barriers` to meet a slab (author places fences).
