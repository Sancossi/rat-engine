---
type: bug
area: Engine
status: Investigating
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
