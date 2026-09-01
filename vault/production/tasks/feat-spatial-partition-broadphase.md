---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Spatial partition broadphase

Intent: GPP Spatial Partition для blockers/events в XZ, когда линейный AABB станет узким местом. Height-grid `SurfaceQuery` не заменяет. Сначала замерить, потом сетка/кваддерево.

Acceptance: query AABB даёт тот же набор коллизий, что линейный скан; порог «берём» — профиль, не догадка.
