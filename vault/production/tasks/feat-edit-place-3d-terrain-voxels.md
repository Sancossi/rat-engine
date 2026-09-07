---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 14
due:
tags: [task]
---

# feat: Edit place 3D terrain voxels

Intent: в Edit ставить и снимать терейн-воксель на выбранном Y (слой), не только `ground_y` клетки. Несколько этажей в одной XZ.

Acceptance: Terrain tool Place voxel / Remove; клик по грани или слой в панели задаёт Y; greybox показывает куб; undo. Не imgui-node-editor. Не вращаемые рампы (следующая карточка).

Depends: [[feat-voxel-occupancy-schema-and-bake|feat: Voxel occupancy schema and bake]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Follow-up: [[chore-edit-voxel-review-polish|chore: Edit voxel review polish]].

## Resolution

Terrain **Place voxel** / **Remove voxel**: `occupancy[]` solid, schema 4→5 при первой записи. Y — слой в панели или грань greybox. Кубы 1 м поверх height-grid. Undo через `EditHistory`. Place cube legacy. Review: Approved.

Verify: Edit → Terrain → Place voxel, два куба в одной XZ; Remove верхнего; Ctrl+Z. `.\build\tests\rat_tests.exe "[viewport],[edit],[height_edit],[terrain]"`.

## Bugs found

none.
