---
type: bug
area: Engine
status: Fixed
review: Approved
severity: High
sprint: Sprint 15
tags: [bug, audit]
---

# Transfer Player map state diverges

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A06.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: transfer from → other меняет GameState.map_id, но EventRuntime сохраняет карту from.

## Expected / acceptance

Неподдерживаемый cross-map transfer отклонён; либо реализован согласованный resolver/переход.


## Resolution

Fixed in eed6158 with reviewed append correction 4eaeaf6. Independent review Approved; full verification 689/689 C++ tests passed and reviewer focused run 19 tests/346 assertions passed. See [[s15-runtime]]. Real UI acceptance remains [[s15-acceptance]].

## Bugs found

No unresolved stage-4 findings.
