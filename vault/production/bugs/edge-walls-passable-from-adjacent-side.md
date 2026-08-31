---
type: bug
area: Engine
status: Open
severity: Medium
sprint: Sprint 6
tags: [bug]
---

# Edge walls passable from adjacent tiles from the side

Origin: chat (Sprint 6). Collision: [[feat: Grid edge barriers]]. Greybox: [[feat: Edit and greybox edge walls]].

Proposed fix: player collision volume as the source of truth for movement hits (see body).

## Repro

1. Open `grey_yard` (or Place cube + Mini/Full fence on one edge).
2. Stand on a **neighbouring** tile that does not own that fence.
3. Walk **along** the fence (parallel to the wall), so the body overlaps the wall from the side rather than walking straight through it.

## Expected

The fence blocks the player's body. You cannot slide through the wall from an adjacent tile.

## Actual

Walk-through from the side. `blocked_by_edge_barriers` only rejects a step when the player **center** crosses the edge plane (`old_x < edge_x` vs `new_x < edge_x`). `half_extent` is used only along the fence length, not as a volume against the wall.

## Notes

Blockers already use an XZ AABB (`player_bounds`). Edge fences and height-grid step-up do not. Proposed: one player collider for all movement collisions.
