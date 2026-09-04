---
type: sprint
status: Done
dates: 2026-09-04/2026-09-18
goal: Voxel 3D terrain — multi-level maps with terrain and rotating ramps
current: false
tags: [sprint]
---

# Sprint 14 — Voxel 3D terrain

Цель: автор строит многоуровневую карту **терейном в 3D**, не height-grid + плиты. Рампы вращаются в пространстве. На `grey_yard` можно зайти на мост.

DoD:

- ADR: воксели вместо «не воксельный мир» из [[feat: Stacked surfaces caves and basements]]
- Схема + parse/dump occupancy (несколько этажей в одной XZ)
- Edit: поставить/снять воксель на Y слоя
- Рампа-клин с поворотом; заход на соседний воксель другого Y
- Play: верх / под пролётом / на мост по рампе; `[collision],[player],[map]` зелёные
- [[Cannot climb onto grey_yard bridge]] Fixed

Вне скоупа: Minecraft-sculpt как продукт, новый renderer, field physics, меши-обязаловка, successor map filename.

Порядок:

- [[research: Voxel 3D terrain ADR]] → [[feat: Voxel occupancy schema and bake]] → [[feat: Edit place 3D terrain voxels]] → [[feat: Rotating ramp voxels]] → [[Cannot climb onto grey_yard bridge]]

Epic: [[feat: Voxel 3D terrain and rotating ramps]]

Roadmap: [[Content vertical slice]]

## Итог

DoD выполнен 2026-09-04. [[ADR-015 Voxel 3D terrain]] Accepted (sparse occupancy 1 м, yaw-клинья, dual-read). Schema 5 `occupancy[]` + bake union. Edit: Place/Remove voxel, Place ramp voxel. [[Cannot climb onto grey_yard bridge]] Fixed: две рампы с запада на плиты, проход снизу сохранён.

## Bugs found

на закрытии: none. Polish вне порядка: [[chore: Occupancy review test polish]], [[chore: Edit voxel review polish]], [[chore: Ramp voxel review polish]], [[chore: Bridge climb review test polish]].
