---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 8
tags: [bug]
---

# Edit edges / ramps only one facing

Origin: chat 2026-09-02 (author): рамки при редактировании ставятся только с одной стороны; нужно уметь с другой. Взято в [[Sprint 8 — Play feel and authoring]].

## Repro

1. Edit, Fence or ramp on a tile.
2. Try to put the barrier / slope on the opposite face of the same cell.

## Expected

Any of N/E/S/W from the clicked edge (or an explicit opposite). Viewport click chooses the face you hit, not a hidden combo default.

## Actual

Viewport Fence/Ladder use `edge_direction_index` (default North). Ramp is ImGui-only (`ramp_direction_index`, default North). There is no pick-the-clicked-edge; opposite side means select the neighbor or hunt the combo.

Proper Sims-like edge paint: [[feat-edit-paint-clicked-edge|feat: Edit paint clicked edge]] (epic [[feat-sims-like-edit-brush-and-edge-paint|feat: Sims-like edit brush and edge paint]]).

Related: [[feat-mouse-viewport-terrain-edit|feat: Mouse viewport terrain edit]], [[feat-edit-greybox-edge-walls|feat: Edit and greybox edge walls]].

## Resolution

Fence и ladder берутся с кликнутой грани клетки, не combo. Противоположная сторона той же клетки работает. Рампа по-прежнему ImGui. Verify: Edit Fence у east/west одной клетки.

## Bugs found

none.
