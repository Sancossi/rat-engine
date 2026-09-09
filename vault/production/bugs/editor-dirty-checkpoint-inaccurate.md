---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Low
sprint: Sprint 15
tags: [bug, audit]
---

# Editor dirty checkpoint inaccurate

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A09.
Подробности и источники: [отчёт](../../../docs/archive/cpp/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: undo к сохранённому состоянию оставляет dirty; повторная compile без изменения тоже ставит dirty.

## Expected / acceptance

Dirty соответствует clean checkpoint; компиляция производных данных не считается правкой.


## Resolution

Implemented in 160a376, independent review Approved. Full verification 672/672 C++ tests passed; reviewer focused run 20 tests/214 assertions. See [[s15-authoring]]. Real desktop interaction acceptance scheduled in [[s15-acceptance]].

## Bugs found

No unresolved implementation findings; build dependency followup resolved in [[msvc-localized-include-dependencies-missed]].
