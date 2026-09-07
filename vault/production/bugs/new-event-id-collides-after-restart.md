---
type: bug
area: Engine
status: Fixed
review: Approved
severity: High
sprint: Sprint 15
tags: [bug, audit]
---

# New event ID collides after restart

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A05.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Карта со stub_1 → новый запуск → Add event снова строит stub_1. API-probe подтверждает успешное добавление и последующий отказ compile.

## Expected / acceptance

Общий allocator для всех трёх UI-путей; отсутствие duplicate ID после reload/restart.


## Resolution

Implemented in 160a376, independent review Approved. Full verification 672/672 C++ tests passed; reviewer focused run 20 tests/214 assertions. See [[s15-authoring]]. Real desktop interaction acceptance scheduled in [[s15-acceptance]].

## Bugs found

No unresolved implementation findings; build dependency followup resolved in [[msvc-localized-include-dependencies-missed]].
