---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Stacked surfaces caves and basements

Intent: несколько walkable поверхностей в одной XZ (мост, пещера, подвал, этаж над этажом). **Не воксельный мир.** Карта остаётся удобной: height-grid / рампы / рёбра на слое; в рантайме они **пекутся** в 3D-solids. Этажи = несколько слоёв сетки с разными Y-диапазонами; пещера/комната = доп. volumes (AABB/mesh), не voxel sculpt. Sprint 3 вынес stacked surfaces: [[Sprint 3 — Height-grid traversal]].

Chat 2026-09-02 (play/author): полноценная многоуровневая карта. Выбор: **плита в воздухе** — верх = пол 2 этажа, низ = потолок 1; стены как сейчас, до плиты. Не парапет по верху Mini/Full забора (это позже, если понадобится). Авторинг: инструмент **Floor slab** — клик по клетке, плита на выбранной высоте (1.6 / 2.0 / своё Y), под ней пусто. Толщина: по умолчанию тонкая (~0.2–0.35), в панели можно задать другую (в т.ч. 1.0).

Acceptance (позже): два пола в одной XZ; цилиндр стоит на опоре в диапазоне Y; потолок — solid; дыра в верхнем полу ведёт вниз; можно поставить airborne tile; по верху стены можно ходить.

Origin: chat (collider design) + chat 2026-09-02 (многоэтажный дом). Depends: [[edge-walls-passable-from-adjacent-side]], [[feat: Bake height-grid and ramps to collision solids]]. Related: [[feat: Field physics puzzles]], [[S3: Height-grid map schema and ground query]], [[feat: Grid edge barriers]].
