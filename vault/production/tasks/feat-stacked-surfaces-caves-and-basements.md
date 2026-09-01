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

Acceptance (позже): два пола в одной XZ; цилиндр стоит на опоре в диапазоне Y; потолок — solid; дыра в верхнем полу ведёт вниз.

Origin: chat (collider design). Depends: [[edge-walls-passable-from-adjacent-side]]. Related: [[feat: Field physics puzzles]], [[S3: Height-grid map schema and ground query]].
