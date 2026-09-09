---
type: task
area: Engine
status: Done
review: Approved
task_type: Chore
sprint: Sprint 15
tags: [task, stabilization]
---

# Geometry and graph fixes

Origin: [[project-audit-followups]].
Contract: [approved implementation plan](../../../docs/archive/cpp/superpowers/plans/2026-09-07-stabilization.md), stage 5.

Remaining Sprint 14 polish and graph self-pin defect, meaningful regression coverage.

Acceptance: stage contract met; meaningful tests passed; independent review Approved; full editor Release rebuilt at stage close.

Depends: [[s15-runtime]].

## Resolution

Included follow-ups: [[chore-ramp-voxel-review-polish]],
[[chore-occupancy-review-test-polish]], [[chore-bridge-climb-review-test-polish]],
[[event-graph-self-pin-drag-stays-armed]].

Implementation passed 697/697 tests. Early independent review found new finite-range overflow in wedge midpoint and float orientation arithmetic; correcting with robust midpoint and double orientation plus large-scale regression before approval. Corrected in 2a44876; full verification 698/698 C++ tests, 8 Python checks and vault passed. Independent review Approved: 6 focused tests / 1,314 assertions passed. Parent full Release build passed; editor: build/dev-release/apps/editor/rat-editor.exe.

## Bugs found

[[slab-ramp-height-mismatch-removes-fence]] reproduced and fixed. Review found intermediate numeric overflow; corrected before commit. No unresolved stage-5 findings. Real GUI gesture acceptance follows in stage 6.


Follow-up reproduced during stabilization: [[slab-ramp-height-mismatch-removes-fence]].
