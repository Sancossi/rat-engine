# Map + Event JSON schema (v1 + v2)

Human-readable schema for map/event data. Files live under `data/maps/<id>.json`.

## Root (`MapData`)

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `schema_version` | int | yes | `1` (legacy flat map) or `2` (height grid + ramps) |
| `id` | string | yes | Map id (e.g. `grey_yard`) |
| `width` | int | yes | Helper grid width in tiles |
| `height` | int | yes | Helper grid height in tiles |
| `tile_size` | number | no | World units per tile (default `1`) |
| `height_grid` | object | v2 only | Elevation grid, required for schema `2` |
| `ramps` | array | v2 only | Optional ramp definitions for schema `2` |
| `edge_barriers` | array | v2 only | Optional tile-edge fences for schema `2` |
| `blockers` | array | no | Static blockers on XZ (legacy full walls + optional height-aware jumpables) |
| `events` | array | no | Event definitions |
| `assets` | array | no | Stable `AssetId` refs (`id`, `kind`, optional `debug_name`). Kinds: `texture`, `audio_clip`, `mesh`, `material`. Gameplay looks up ids in `AssetRegistry`; omit when unused. |

### HeightGrid (v2)

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `origin_x` | int | yes | World tile X of `ground_y[0]` |
| `origin_z` | int | yes | World tile Z of `ground_y[0]` |
| `width` | int | yes | Grid width in tiles |
| `height` | int | yes | Grid height in tiles |
| `ground_y` | array<number> | yes | Row-major tile ground heights, length `width * height` |

`ground_y` index: `index = (z - origin_z) * width + (x - origin_x)`.

### EdgeBarrierDef (v2)

Optional fence on one edge of a height-grid tile, sitting on that tile's `ground_y`. The edge is collision only — not standable support.

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `tile` | object | yes | `{ "x": int, "z": int }` owner tile |
| `direction` | string | yes | `north`, `east`, `south`, `west` (same strings as ramps) |
| `height` | number | yes | Fence height above owner `ground_y`; must be `> 0` |

```json
{ "tile": { "x": 0, "z": 0 }, "direction": "east", "height": 0.45 }
```

Loader canonicalize (after parse, schema v2):

- Unique key `(tile, direction)`: last wins.
- Drop barriers whose tile has a ramp.
- Drop out-of-grid tiles and `height <= 0` (do not fail the map).
- Do not merge opposite encodings (east of A and west of the neighbor stay separate).

Walk across the edge is blocked while feet are below `owner_top + height`. Mini `0.45` is jumpable with default `JumpTuning`; full `1.6` is not.

### RampDef (v2)

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `tile` | object | yes | `{ "x": int, "z": int }` tile coordinate |
| `direction` | string | yes | `north`, `east`, `south`, `west` |
| `low_y` | number | yes | Height at low edge of the ramp |
| `high_y` | number | yes | Height at high edge of the ramp, must be `>= low_y` |

### Blocker

```json
{ "min_x": 3, "min_z": -1, "max_x": 5, "max_z": 1 }
```

Legacy shape above remains valid and represents a full non-jumpable wall.
Height-aware low obstacle shape extends it with optional vertical keys:

```json
{
  "min_x": 3,
  "min_z": -1,
  "max_x": 5,
  "max_z": 1,
  "base_y": 0.0,
  "top_y": 0.75,
  "jumpable": true
}
```

Notes:
- `jumpable: false` behaves as a full wall (always blocks horizontal traversal).
- `base_y` and `top_y` are a required pair: if one is specified, the other must be specified too.
- `top_y` must be greater than or equal to `base_y`.
- `jumpable: true` requires `base_y` + `top_y`.
- For jumpable blockers, the blocker acts as a vertical slab and blocks only while feet world-Y is in `[base_y, top_y)`.
- Feet below `base_y` pass through (future bridge compatibility), feet at/above `top_y` pass through.

## Event

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `id` | string | yes | Unique on map |
| `tile` | object | no | `{ "x": int, "z": int }` snap helper |
| `volume` | object | no | AABB same as blocker; trigger volume |
| `pages` | array | yes | At least one page preferred |

At least one of `tile` / `volume` should be present for interactable events.

## Page

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `trigger` | string | yes | See triggers |
| `conditions` | array | no | All must pass (AND) |
| `commands` | array | no | Ordered RM-like list |

### Triggers

`action` | `player_touch` | `event_touch` | `autorun` | `parallel`

### Conditions

Discriminated by `type`:

```json
{ "type": "switch", "id": 1, "value": true }
{ "type": "variable", "id": 2, "op": ">=", "value": 3 }
{ "type": "item", "id": "door_key", "quantity": 1 }
{ "type": "self_switch", "key": "A", "value": true }
```

Variable `op`: `==` `!=` `<` `<=` `>` `>=`

Self-switch `key`: `A` `B` `C` `D`

### Commands

Discriminated by `op`:

| `op` | Fields |
|------|--------|
| `show_text` | `text` (string) |
| `control_switch` | `id` (uint), `value` (bool) |
| `control_variable` | `id` (uint), `value` (int) — set absolute for v1 |
| `conditional_branch` | `condition` (one condition object), `then` (commands[]), optional `else` (commands[]) |
| `wait` | `frames` (int ≥ 0) |
| `transfer_player` | `map_id` (string), `x`, `y`, `z` (numbers) |
| `change_items` | `id` (string), `delta` (int), optional `key_item` (bool) |
| `play_se` | `id` (string, required, non-empty cue id; stored in `Command::text`) |
| `comment` | `text` (string) |

Nested `parallel` start from a Parallel page is **not** allowed at runtime (ADR-008); the loader still accepts the command list shape for later validation in the event VM.

## Example

See `data/maps/grey_yard.json`.

## Backward compatibility

- Loader accepts `schema_version: 1` and builds an implicit flat `height_grid`:
  - `origin_x = 0`, `origin_z = 0`
  - `width = map.width`, `height = map.height`
  - every `ground_y` value is `0`
- Loader accepts `schema_version: 2` and reads explicit `height_grid` + optional `ramps` + optional `edge_barriers`.
- For `schema_version: 1`, `edge_barriers` is ignored if present.
- For `schema_version: 2`, invalid `ground_y` length (not equal to `width * height`) is rejected.
