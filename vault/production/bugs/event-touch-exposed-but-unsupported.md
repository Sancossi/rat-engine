---
type: bug
area: Engine
status: Investigating
review: In review
severity: Medium
sprint: Sprint 15
tags: [bug, audit]
---

# Event touch exposed but unsupported

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A10.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Event System и схема перечисляют event_touch; why_not_fired всегда возвращает WrongPage для него.

## Expected / acceptance

Явная диагностика unsupported в authoring/validation либо реализованный trigger с приёмкой.
