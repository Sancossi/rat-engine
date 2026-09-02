---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Edit paint clicked edge

Intent: рамка/забор/лестница на **той грани**, по которой кликнули (N/E/S/W), не combo по умолчанию. Drag вдоль ребра красит стену как в Sims. Закрывает [[edit-edges-only-one-facing]].

Acceptance: click near an edge upserts that facing; opposite side of the same cell works without selecting the neighbor. Hold-drag along a wall line is one undo group. Ramp opposite facing: same pick (or explicit flip) if still ImGui-only.

Origin: [[feat: Sims-like edit brush and edge paint]]. Related: [[feat: Edit hold-drag brush]], [[research: Sims build-mode edit analog]]. Взято в [[Sprint 8 — Play feel and authoring]].

## Resolution

Fence Mini/Full/Remove и Place ladder: клик у грани клетки выбирает N/E/S/W (`nearest_tile_edge`); drag вдоль рёбер — один undo. Рампы по-прежнему ImGui combo. Verify: Edit Fence у восточной грани; `.\build\tests\rat_tests.exe "[viewport_edit]"`.

## Bugs found

none. Ramp facing stays combo (out of this slice).
