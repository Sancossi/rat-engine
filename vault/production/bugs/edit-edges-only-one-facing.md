---
type: bug
area: Engine
status: Open
severity: Medium
sprint:
tags: [bug]
---

# Edit edges / ramps only one facing

Origin: chat 2026-09-02 (author): рамки при редактировании ставятся только с одной стороны; нужно уметь с другой.

## Repro

1. Edit, Fence or ramp on a tile.
2. Try to put the barrier / slope on the opposite face of the same cell.

## Expected

Any of N/E/S/W from the clicked edge (or an explicit opposite). Viewport click chooses the face you hit, not a hidden combo default.

## Actual

Viewport Fence/Ladder use `edge_direction_index` (default North). Ramp is ImGui-only (`ramp_direction_index`, default North). There is no pick-the-clicked-edge; opposite side means select the neighbor or hunt the combo.

Proper Sims-like edge paint: [[feat: Edit paint clicked edge]] (epic [[feat: Sims-like edit brush and edge paint]]).

Related: [[feat: Mouse viewport terrain edit]], [[feat: Edit and greybox edge walls]].
