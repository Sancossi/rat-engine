---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 15
tags: [bug, audit]
---

# Replay parser accepts unsupported format

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A08.
Подробности и источники: [отчёт](../../../docs/archive/cpp/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: пустой объект и schema_version=999 принимаются; replay helper не использует header.dt и не проверяет map ID.

## Expected / acceptance

Строгая валидация версии/header/ticks, согласованный dt и отдельный результат ошибки.


## Resolution

Fixed in eed6158 with reviewed append correction 4eaeaf6. Independent review Approved; full verification 689/689 C++ tests passed and reviewer focused run 19 tests/346 assertions passed. See [[s15-runtime]]. Real UI acceptance remains [[s15-acceptance]].

## Bugs found

No unresolved stage-4 findings.
