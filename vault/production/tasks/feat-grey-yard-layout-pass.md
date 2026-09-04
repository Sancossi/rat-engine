---
type: task
area: Game
status: In progress
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
- Лофт `y: 2` и gantry `y: 1` не делят одну клетку. Рампа и высокий край с целой сеткой (после [[Ramp greybox fill breaks cells and hides the high side]]).
- Использовать мосты / рампы с граней / indoor dim, когда эти карточки закрыты: [[feat: Walkable bridges over open ground]], [[feat: Place ramp from clicked tile edge]], [[feat: Indoor volume dims the street]]. Indoor AABB inset к внутренним тайлам (центр квада на грани volume не димится).
- Обновить `grey_yard.json` + headless/`[quest]`/`[smoke]`, если тайлы съехали. Не successor map, не меши, не field physics.

Origin: chat 2026-09-04 (закрытие Sprint 12, «изменения по карте»). Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[Define vertical slice scope]], [[Content vertical slice]]. Depends: [[feat: Place ramp from clicked tile edge]], [[feat: Walkable bridges over open ground]], [[feat: Indoor volume dims the street]].

## Resolution

## Bugs found
