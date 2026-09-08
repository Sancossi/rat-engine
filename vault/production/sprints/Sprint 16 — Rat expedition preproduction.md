---
type: sprint
status: Done
dates: 2026-09-08
goal: Install BMad and prepare the new rat RPG design and bounded release queue
current: false
tags: [sprint, preproduction]
---

# Sprint 16 — Rat expedition preproduction

Origin: User-approved [preproduction plan](../../../docs/rat-expedition-preproduction-plan.md).

## Ordered work

1. [[chore-bmad-vault-integration]] — install and verify the process.
2. [[design-rat-expedition-preproduction]] — design, technical comparison and release queue.

## Boundaries

Documentation and process integration only. Engine adoption and game implementation
belong to subsequent stages. Do not resume historical sprint queues.

## Resolution

Both ordered cards are Done with review Approved after independent read-only
review. BMad Core 6.12.0 + GDS v0.7.2, canonical game design and 11 future cards
are ready; engine choice and runtime implementation remain subsequent work.

Full Windows Release configure/build/CTest completed with exit 0: 714 C++ tests,
16 Python tests and vault checks passed. Editor:
`C:/5_gamedev/rat-engine/build/dev-release/apps/editor/rat-editor.exe`.
[Acceptance evidence and limits](../../../docs/audits/2026-09-08-rat-expedition-preproduction.md).

All 40 initial and seven later external files preserved. No current sprint;
board uses `__NO_ACTIVE_SPRINT__`. Next: [[expedition-engine-decision]].

## Bugs found

none remaining. Setup/resolver mismatches and a new empty duplicate roadmap
navigation note were corrected before closure. No runtime fixes or GUI/playtest
claims. See acceptance evidence for details.
