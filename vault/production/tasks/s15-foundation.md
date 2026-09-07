---
type: task
area: Engine
status: Done
review: Approved
task_type: Chore
sprint: Sprint 15
tags: [task, stabilization]
---

# Foundation and workflow

Origin: [[project-audit-followups]].
Contract: [approved implementation plan](../../../docs/superpowers/plans/2026-09-07-stabilization.md), stage 1.

A11–A14: shared instructions, project skills, vault links/status checker and board, actual docs, reproducible build wrapper/presets.

Acceptance: stage contract met; meaningful tests passed; independent review Approved; full editor Release rebuilt at stage close.

## Resolution

Implemented in c11288b; documentation corrections in 83746e7. Independent reviewer Approved after corrections. Fresh Windows preset configure/build, 643/643 C++ tests, 8/8 Python checks and vault validation passed. Three project skills validated. Parent stage-close full Release build passed (cmake --build --preset dev-release, exit 0) after storage checkpoint; editor: build/dev-release/apps/editor/rat-editor.exe.

## Bugs found

Review caught inaccurate graph-runtime wording; corrected before approval. No unresolved foundation findings.


Follow-up found during header-change verification: [[msvc-localized-include-dependencies-missed]].
