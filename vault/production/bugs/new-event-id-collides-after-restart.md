---
type: bug
area: Engine
status: Open
severity: High
sprint:
tags: [bug, audit]
---

# New event ID collides after restart

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A05.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Карта со stub_1 → новый запуск → Add event снова строит stub_1. API-probe подтверждает успешное добавление и последующий отказ compile.

## Expected / acceptance

Общий allocator для всех трёх UI-путей; отсутствие duplicate ID после reload/restart.
