---
type: bug
area: Engine
status: Investigating
review: In review
severity: High
sprint: Sprint 15
tags: [bug, audit]
---

# Map validation accepts NaN

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A04.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: MapData.tile_size=NaN проходит compile и serialize; reload собственного JSON не проходит.

## Expected / acceptance

Все геометрические значения проверены; допустимые карты проходят serialize/load.
