---
type: bug
area: Engine
status: Investigating
review: In review
severity: High
sprint: Sprint 15
tags: [bug, audit]
---

# File write truncates existing save

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A02.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

OsFileStore открывает целевой файл с trunc до завершения записи. Риск подтверждён кодом, авария диска не моделировалась.

## Expected / acceptance

Ошибка записи до замены сохраняет прежнюю карту/save; временный файл и корректная замена.
