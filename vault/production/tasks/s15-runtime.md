---
type: task
area: Engine
status: In progress
review: Needs fixes
task_type: Chore
sprint: Sprint 15
tags: [task, stabilization]
---

# Runtime contracts and replay

Origin: [[project-audit-followups]].
Contract: [approved implementation plan](../../../docs/superpowers/plans/2026-09-07-stabilization.md), stage 4.

A06–A08/A10: unsupported event diagnostics and defenses, complete versioned replay contracts/checksum.

Acceptance: stage contract met; meaningful tests passed; independent review Approved; full editor Release rebuilt at stage close.

Depends: [[s15-authoring]].

## Resolution

Follow-up: [[explicit-empty-graph-revives-legacy-commands]].

Implemented in eed6158. Full Windows verification passed: 687/687 C++ tests, 8 Python checks and vault. Review found quadratic streaming record_tick validation of all prior ticks; correcting before final approval. Parent full Release build passed, exit 0. Final independent review continues. Runtime/replay contract documented in docs/adr-replay-v2.md.

## Bugs found

Pending.
