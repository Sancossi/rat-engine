---
type: bug
area: Engine
status: Fixed
review: Approved
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


## Resolution

Fixed in ebe7759 and c90638f; independent review Approved. Full Windows verification: 658/658 C++ tests, 8 Python checks and vault passed. See [[s15-storage]] for stage evidence.

## Bugs found

Validation review followups resolved in c90638f; no unresolved findings.
