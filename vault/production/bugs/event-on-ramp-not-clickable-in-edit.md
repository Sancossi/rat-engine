---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 12
tags: [bug]
---

# Event on ramp is not mouse-clickable in Edit

Origin: chat 2026-09-04 (edit). Pick: [[feat-mouse-viewport-map-edit|feat: Mouse viewport map edit]] / `unproject_to_ground_plane(..., ground_y = 0)`. Related: [[ramp-greybox-fill-breaks-cells|Ramp greybox fill breaks cells and hides the high side]].

## Repro

1. Edit → Events. Put an event on a ramp tile (e.g. `grey_yard` `(8, 8)`).
2. Click the cyan marker / tile with the mouse (Select).

## Expected

The event is selected (same as a ground-level tile).

## Actual

Click does not hit the event. `handle_edit_mouse_input` always unprojects onto `y = 0`, so the ortho ray’s XZ on a sloped/raised surface is not the ramp tile. `pick_map_object_xz` then tests that XZ against the event AABB.

## Resolution

`unproject_to_terrain` бьёт лучом по квадам height-grid/рамп; редактор больше не сажает клик на y=0. Кубы y=1 тоже. Verify: Edit → Events, клик по маркеру на рампе (8,8); `.\build\tests\rat_tests.exe "[viewport_edit],[terrain]"`. Review: Approved.

## Bugs found

none.
