# Map + Event JSON schema (v1)

Human-readable schema for Sprint 1. Files live under `data/maps/<id>.json`.

## Root (`MapData`)

| Field | Type | Required | Notes |
|-------|------|----------|-------|
| `schema_version` | int | yes | Must be `1` |
| `id` | string | yes | Map id (e.g. `grey_yard`) |
| `width` | int | yes | Helper grid width in tiles |
| `height` | int | yes | Helper grid height in tiles |
| `tile_size` | number | no | World units per tile (default `1`) |
| `blockers` | array | no | Static AABB blockers on XZ |
| `events` | array | no | Event definitions |

### Blocker

```json
{ "min_x": 3, "min_z": -1, "max_x": 5, "max_z": 1 }
```

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
| `comment` | `text` (string) |

Nested `parallel` start from a Parallel page is **not** allowed at runtime (ADR-008); the loader still accepts the command list shape for later validation in the event VM.

## Example

See `data/maps/grey_yard.json`.
