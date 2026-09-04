---
type: bug
area: Engine
status: Investigating
severity: Medium
sprint: Sprint 12
tags: [bug]
---

# Event on ramp is not mouse-clickable in Edit

Origin: chat 2026-09-04 (edit). Pick: [[feat: Mouse viewport map edit]] / `unproject_to_ground_plane(..., ground_y = 0)`. Related: [[Ramp greybox fill breaks cells and hides the high side]].

## Repro

1. Edit → Events. Put an event on a ramp tile (e.g. `grey_yard` `(8, 8)`).
2. Click the cyan marker / tile with the mouse (Select).

## Expected

The event is selected (same as a ground-level tile).

## Actual

Click does not hit the event. `handle_edit_mouse_input` always unprojects onto `y = 0`, so the ortho ray’s XZ on a sloped/raised surface is not the ramp tile. `pick_map_object_xz` then tests that XZ against the event AABB.
