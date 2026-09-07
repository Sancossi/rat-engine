---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 5
due:
tags: [task]
---

# feat: Grid edge barriers

Intent: забор на ребре клетки `{tile, direction, height}` над верхом владельца. Мини vs полная — одна высота vs апекс прыжка (~1.17). Ребро не support. Origin: [[feat-terrain-wall-cubes|feat: Terrain wall cubes]].

Acceptance: schema v2 `edge_barriers` (без bump v3); лоадер канонизирует дубликаты и режет ramp-клетки; ходьба через ребро блокируется; 0.45 перепрыгивается; 1.6 нет даже с полным hold.

Depends: [[feat-terrain-wall-cubes|feat: Terrain wall cubes]]. Next: [[feat-edit-greybox-edge-walls|feat: Edit and greybox edge walls]]. Взято в [[Sprint 5 — Terrain and edge walls]].

## Resolution

Schema v2 `edge_barriers` `{tile, direction, height}` (no v3). Loader last-wins on `(tile, direction)`, drops ramp tiles / OOB / `height <= 0`. Walk blocked below `owner_top + height`; 0.45 jumpable, 1.6 not, with default JumpTuning. Edges are not support. Review: Approved.

Verify: `.\build\tests\rat_tests.exe "[map]"` and `"[edge]"`.

Follow-up: [[feat-edit-greybox-edge-walls|feat: Edit and greybox edge walls]]
Follow-up: [[edge-walls-passable-from-adjacent-side]]

## Bugs found

[[edge-walls-passable-from-adjacent-side]]
