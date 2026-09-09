---
type: task
area: Engine
status: Done
review: Approved
task_type: Chore
sprint: Sprint 15
tags: [task, stabilization]
---

# Safe storage and validation

Origin: [[project-audit-followups]].
Contract: [approved implementation plan](../../../docs/archive/cpp/superpowers/plans/2026-09-07-stabilization.md), stage 2.

A02–A04: atomic writes with one backup, transactional strict save v2/legacy reader, finite map validation and regressions.

Acceptance: stage contract met; meaningful tests passed; independent review Approved; full editor Release rebuilt at stage close.

Depends: [[s15-foundation]].

## Resolution

Implemented in ebe7759. Full Windows verification passed: 656/656 C++ tests, Python checks and vault validation. Independent review Needs fixes: malformed graph params silently default; derived ramp/voxel heights can overflow despite finite inputs. Isolated reproductions: build/stabilization/review-storage/probe.cpp. Corrected in c90638f; full verification 658/658 C++ tests, 8 Python checks and vault passed. Independent re-review Approved: isolated malformed-parameter and geometry-overflow probes now reject all cases. Parent stage-close full Release build passed (cmake --build --preset dev-release, exit 0); editor: build/dev-release/apps/editor/rat-editor.exe.

## Bugs found

Review found two additional validation bypasses; both corrected in c90638f and independently reproduced as rejected. No unresolved storage findings.
