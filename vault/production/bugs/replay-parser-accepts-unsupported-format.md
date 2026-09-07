---
type: bug
area: Engine
status: Investigating
severity: Medium
sprint: Sprint 15
tags: [bug, audit]
---

# Replay parser accepts unsupported format

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A08.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: пустой объект и schema_version=999 принимаются; replay helper не использует header.dt и не проверяет map ID.

## Expected / acceptance

Строгая валидация версии/header/ticks, согласованный dt и отдельный результат ошибки.
