---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 15
tags: [bug, audit]
---

# Event touch exposed but unsupported

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A10.
Подробности и источники: [отчёт](../../../docs/archive/cpp/audits/2026-09-07-project-review.md).

## Repro / evidence

Event System и схема перечисляют event_touch; why_not_fired всегда возвращает WrongPage для него.

## Expected / acceptance

Явная диагностика unsupported в authoring/validation либо реализованный trigger с приёмкой.


## Resolution

Fixed in eed6158 with reviewed append correction 4eaeaf6. Independent review Approved; full verification 689/689 C++ tests passed and reviewer focused run 19 tests/346 assertions passed. See [[s15-runtime]]. Real UI acceptance remains [[s15-acceptance]].

## Bugs found

No unresolved stage-4 findings.
