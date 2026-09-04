---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
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

Origin: chat 2026-09-04 (плейтест моста; «переделать терейн на воксели», «вращающиеся рампы», «строить терейном многоуровневую карту»). Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[Cannot climb onto grey_yard bridge]], [[feat: Walkable bridges over open ground]], [[feat: Stacked surfaces caves and basements]], [[feat: Place ramp from clicked tile edge]], [[Content vertical slice]].

## Resolution

## Bugs found
