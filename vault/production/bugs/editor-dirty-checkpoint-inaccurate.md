---
type: bug
area: Engine
status: Investigating
severity: Low
sprint: Sprint 15
tags: [bug, audit]
---

# Editor dirty checkpoint inaccurate

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A09.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: undo к сохранённому состоянию оставляет dirty; повторная compile без изменения тоже ставит dirty.

## Expected / acceptance

Dirty соответствует clean checkpoint; компиляция производных данных не считается правкой.
