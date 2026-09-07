---
type: bug
area: Engine
status: Investigating
severity: High
sprint:
tags: [bug, audit]
---

# Save parser accepts incomplete and NaN

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A03.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: RATSAVE1 без данных и pos nan 0 0 принимаются. Item ID с пробелом ломает roundtrip.

## Expected / acceptance

Обязательные поля и finite-check; единый контракт ID; ошибка не изменяет прежнее состояние.
