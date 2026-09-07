---
type: bug
area: Engine
status: Open
severity: High
sprint:
tags: [bug, audit]
---

# Transfer Player map state diverges

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A06.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: transfer from → other меняет GameState.map_id, но EventRuntime сохраняет карту from.

## Expected / acceptance

Неподдерживаемый cross-map transfer отклонён; либо реализован согласованный resolver/переход.
