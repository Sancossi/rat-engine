---
type: task
area: Engine
status: In review
review: In review
task_type: Chore
sprint: Sprint 15
tags: [task, stabilization]
---

# Safe storage and validation

Origin: [[project-audit-followups]].
Contract: [approved implementation plan](../../../docs/superpowers/plans/2026-09-07-stabilization.md), stage 2.

A02–A04: atomic writes with one backup, transactional strict save v2/legacy reader, finite map validation and regressions.

Acceptance: stage contract met; meaningful tests passed; independent review Approved; full editor Release rebuilt at stage close.

Depends: [[s15-foundation]].

## Resolution

Implemented in ebe7759. Full Windows verification passed: 656/656 C++ tests, Python checks and vault validation. Independent review Needs fixes: malformed graph params silently default; derived ramp/voxel heights can overflow despite finite inputs. Isolated reproductions: build/stabilization/review-storage/probe.cpp. Corrected in c90638f; full verification 658/658 C++ tests, 8 Python checks and vault passed. Re-review pending.

## Bugs found

Pending.
