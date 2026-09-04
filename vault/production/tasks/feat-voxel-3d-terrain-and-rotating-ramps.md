---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 14
due:
tags: [task]
---

# feat: Voxel 3D terrain and rotating ramps

Intent: строить **многоуровневую** карту самим терейном в 3D, не height-grid + отдельные плиты. Playtest: на мост `grey_yard` не залезть — 2.5D клетка либо сплошной куб до Y=0, либо airborne slab без подхода. Нужны воксели в 3D-пространстве и **вращаемые** рампы (ориентация куска в пространстве, не крутящийся prop).

Это меняет решение [[feat: Stacked surfaces caves and basements]] («не воксельный мир»). Сначала ADR, потом схема/Edit/коллизия. Не field physics, не меши как обязательный арт.

Acceptance:

- Edit: ставить/снимать терейн-воксели в 3D (несколько этажей в одной XZ), не только `ground_y` клетки.
- Рампа — воксель/клин с поворотом (не только N/E/S/W на одном слое сетки); с неё заходят на соседний воксель другого Y.
- Play: ходить по верху, под пролётом, между этажами по рампе/лестнице без «телепорта на землю».
- Greybox + bake в collision solids. `grey_yard` можно собрать как многоэтажный двор (мост с заходом).
- Unit/headless на два этажа + заход на пролёт. Не Minecraft-sculpt как продукт, не новый renderer.

Split for [[Sprint 14 — Voxel 3D terrain]]: [[research: Voxel 3D terrain ADR]] → [[feat: Voxel occupancy schema and bake]] → [[feat: Edit place 3D terrain voxels]] → [[feat: Rotating ramp voxels]] → [[Cannot climb onto grey_yard bridge]]. Эта карточка — epic, не брать в работу отдельно.

## Resolution

Sprint 14 закрыл epic: [[ADR-015 Voxel 3D terrain]] (sparse occupancy 1 м + yaw-клинья); schema 5 parse/dump/bake; Edit Place/Remove voxel и Place ramp voxel; `grey_yard` заход на мост двумя клиньями без потери прохода снизу. Review цепочки: Approved.

Verify: Edit → Terrain → Place voxel / Place ramp voxel; Play заход на мост с запада. `.\build\tests\rat_tests.exe "[map],[collision],[player],[viewport],[edit]"`.

## Bugs found

none. Follow-up polish (не DoD): [[chore: Occupancy review test polish]], [[chore: Edit voxel review polish]], [[chore: Ramp voxel review polish]], [[chore: Bridge climb review test polish]].
