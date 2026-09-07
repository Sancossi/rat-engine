---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 13
due:
tags: [task]
---

# feat: Grey yard layout pass

Intent: `grey_yard` после height-grid / лофта / gantry читается как куча клеток, а не двор-мастерская. Нужен проход геометрии и маркеров, не новый filename.

Acceptance:

- Три читаемые зоны на одной карте: **двор** (spawn, crates, scrap, foreman, apprentice, walker), **лофт** (лестница + плиты + `loft_plank`), **gantry** (рампа 0→1, jumpable, `elevated_after_blocker`). Зоны не перекрывают друг друга по XZ так, что с земли не понять, куда идти.
- Квест cog (switches 1/2, `rusty_cog`) остаётся на земле; обход crates north/south живой. Edit-жест «отодвинуть crate» всё ещё работает.
- Лофт `y: 2` и gantry `y: 1` не делят одну клетку. Рампа и высокий край с целой сеткой (после [[ramp-greybox-fill-breaks-cells|Ramp greybox fill breaks cells and hides the high side]]).
- Использовать мосты / рампы с граней / indoor dim, когда эти карточки закрыты: [[feat-walkable-bridges-over-open-ground|feat: Walkable bridges over open ground]], [[feat-place-ramp-from-clicked-tile-edge|feat: Place ramp from clicked tile edge]], [[feat-indoor-volume-dims-the-street|feat: Indoor volume dims the street]]. Indoor AABB inset к внутренним тайлам (центр квада на грани volume не димится).
- Обновить `grey_yard.json` + headless/`[quest]`/`[smoke]`, если тайлы съехали. Не successor map, не меши, не field physics.

Origin: chat 2026-09-04 (закрытие Sprint 12, «изменения по карте»). Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[define-vertical-slice-scope|Define vertical slice scope]], [[content-vertical-slice|Content vertical slice]]. Depends: [[feat-place-ramp-from-clicked-tile-edge|feat: Place ramp from clicked tile edge]], [[feat-walkable-bridges-over-open-ground|feat: Walkable bridges over open ground]], [[feat-indoor-volume-dims-the-street|feat: Indoor volume dims the street]].

## Resolution

`grey_yard` schema 4: три зоны (двор / лофт y=2 / gantry y=1). Дом на западе с inset indoor AABB; мост-плиты `(6–8,5)` над dirt; вторая рампа gantry `(9,7)` north. Южный обход crates со спавна восстановлен (дом сдвинут на запад; тест цилиндра spawn → юг → scrap). Русские `show_text` и cog-квест на земле. Review: Approved (после фикса bypass).

Verify: Play — обойти ящики с юга; зайти в дом (улица тускнеет); пройти под мостом; две рампы на эстакаду. `.\build\tests\rat_tests.exe "[quest],[smoke],[map]"`.

## Bugs found

Playtest: [[cannot-climb-onto-grey-yard-bridge|Cannot climb onto grey_yard bridge]]. Follow-up: [[feat-voxel-3d-terrain-and-rotating-ramps|feat: Voxel 3D terrain and rotating ramps]].
