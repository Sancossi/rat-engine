---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Stacked surfaces caves and basements

Intent: несколько walkable поверхностей в одной XZ (мост, пещера, подвал, этаж над этажом). **Не воксельный мир.** Карта остаётся удобной: height-grid / рампы / рёбра на слое; в рантайме они **пекутся** в 3D-solids. Этажи = несколько слоёв сетки с разными Y-диапазонами; пещера/комната = доп. volumes (AABB/mesh), не voxel sculpt. Sprint 3 вынес stacked surfaces: [[Sprint 3 — Height-grid traversal]].

Chat 2026-09-02 (play/author): полноценная многоуровневая карта. Выбор: **плита в воздухе** — верх = пол 2 этажа, низ = потолок 1; стены как сейчас, до плиты. Не парапет по верху Mini/Full забора (это позже, если понадобится). Авторинг: инструмент **Floor slab** — клик по клетке, плита на выбранной высоте (1.6 / 2.0 / своё Y), под ней пусто. Толщина: по умолчанию тонкая (~0.2–0.35), в панели можно задать другую (в т.ч. 1.0). Хранение: список плит в карте (как рампы), не второй height-grid. Физика: стоять и ходить по **коллайдерам** (AABB плиты + рампы как солиды), не по второму `SurfaceQuery`. Лестницы/пожарные: вертикальный подъём и спуск. Авторинг как ребро (клетка + сторона + y_lo/y_hi). Управление: подошёл, вперёд/назад — едешь по Y, прыжок не нужен; отпустил — сойти на пол или упасть.

Acceptance (позже): два пола в одной XZ; цилиндр стоит на опоре в диапазоне Y; потолок — solid; дыра в верхнем полу ведёт вниз; можно поставить airborne tile. Рампы и земля тоже пекутся в walkable солиды (верх/клин), не только боковые грани. По лестнице можно подняться и спуститься.

Origin: chat (collider design) + chat 2026-09-02 (многоэтажный дом). Depends: [[edge-walls-passable-from-adjacent-side]], [[feat: Bake height-grid and ramps to collision solids]]. Related: [[feat: Field physics puzzles]], [[S3: Height-grid map schema and ground query]], [[feat: Grid edge barriers]].

Spec: `docs/superpowers/specs/2026-09-02-stacked-floors-colliders-design.md`
Plan: `docs/superpowers/plans/2026-09-02-stacked-floors-colliders.md`
Follow-up: [[feat: Step off ladder onto same-tile floor]], [[feat: Ladder toward-climb, jump grab, jump off]], [[events-match-ground-while-on-slab]]

## Resolution

Schema v3: `floor_slabs` и `ladders`. Play при `map != nullptr` стоит/падает/бьётся о потолок по baked solids (земля, призмы рамп, плиты); лестница — overlap, в грань = +Y. Edit: инструменты Floor slab (toggle) и Ladder (last-wins) + greybox. `grey_yard` — schema 3: лофт `top_y` 2.0 на клетках (0–2,4) и (0–1,5) (дыра в (2,5)), лестница east на (2,4) `y` 0–2. Verify: `rat_tests.exe "[collision]"` / `"[unit][player]"` / `"[unit][edit]"` / `"[smoke]"`; в Play подойти с востока к лестнице у (2,4), подняться, сойти в дыру. Review: Approved (финальный срез + re-review потолка/`ramp_index`).

## Bugs found

- [[events-match-ground-while-on-slab]]
- [[feat: Step off ladder onto same-tile floor]]

