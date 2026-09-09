---
type: sprint
status: Done
dates: 2026-09-07/2026-09-07
goal: Stabilize data integrity, authoring, replay, workflow and automated desktop acceptance
current: false
tags: [sprint]
---

# Sprint 15 — Stabilization

Approved implementation: [plan](../../../docs/archive/cpp/superpowers/plans/2026-09-07-stabilization.md).
Epic: [[project-audit-followups]].

Order:

- [[s15-foundation]]
- [[s15-storage]]
- [[s15-authoring]]
- [[s15-runtime]]
- [[s15-geometry]]
- [[s15-acceptance]]

DoD: A01–A16 and included Sprint 14 followups verified; independent review per stage, full Release build, automated GUI suite and package checks. No scope expansion into new gameplay or asset importer.

## Resolution

Approved implementation complete; all six stage cards independently approved.
Local Windows Release and fresh headless each passed 707 tests; full GUI and
installed ZIP each passed 40 scenarios on verified WARP. Clang Analyzer 45/45,
Python 10/10, vault, actionlint and the four-map benchmark passed. Full Release
editor rebuilt after each stage. [Final evidence and limitations](../../../docs/archive/cpp/audits/2026-09-07-stabilization-evidence.md).

CI includes Linux, ASan/UBSan and source-absent downloaded-package jobs, whose
execution remains unobserved; configuration is not reported as a passed run.
There is now no active sprint; the Current sprint board uses its empty sentinel.

## Bugs found

Storage numeric/parameter bypasses, empty-graph migration, MSVC dependency
discovery, slab/ramp height mismatch and GUI input/scale/graph gesture defects
were fixed and reviewed within the sprint. See stage cards for reproductions
and evidence. No unresolved in-scope implementation findings.
