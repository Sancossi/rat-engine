---
type: bug
area: Engine
status: Fixed
review: Approved
severity: High
sprint: Sprint 15
tags: [bug, audit]
---

# Editor discards unsaved map

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A01.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Edit → изменить карту → F5/Reload/закрытие. По коду отсутствует dirty guard; GUI-проход ещё нужен.

## Expected / acceptance

Единый Save/Discard/Cancel на reload и закрытие, отмена при ошибке записи.


## Resolution

Implemented in 160a376, independent review Approved. Full verification 672/672 C++ tests passed; reviewer focused run 20 tests/214 assertions. See [[s15-authoring]]. Real desktop interaction acceptance scheduled in [[s15-acceptance]].

## Bugs found

No unresolved implementation findings; build dependency followup resolved in [[msvc-localized-include-dependencies-missed]].
