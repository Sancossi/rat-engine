---
type: sprint
status: In progress
dates: 2026-09-02/2026-09-15
goal: Play feel on grey_yard plus Sims-like edit and event node-graph authoring
current: true
tags: [sprint]
---

# Sprint 8 — Play feel and authoring

Цель: после playtest 2026-09-02 закрыть камеру на лестнице, падение с плиты, кисть/рамки в Edit и авторство событий графом нод. Runtime событий остаётся bytecode.

DoD:

- Climb-камера за игроком на гранях (в т.ч. East с `(3, 5)`); поворот на lock/dismount и `C` плавный
- Сход со 2 этажа / в дыру — Fall с гравитацией, без телепорта на землю
- Edit: hold-drag кисть по клеткам; забор/лестница по кликнутому ребру (все стороны)
- Event: research + модель/compile + холст в Edit; Play тот же interpreter

Вне скоупа: свободный orbit мышкой, Ruby/JS VM, полный набор MZ-команд, каталог объектов Sims, новый спринт архитектуры.

Эпики (не в очереди pickup): [[feat: Sims-like edit brush and edge paint]], [[feat: Event node graph authoring]]. Баг [[edit-edges-only-one-facing]] закрывается слайсом paint-clicked-edge.

Порядок:

- [[climb-camera-locks-on-far-side]] → [[feat: Smooth camera turn]] → [[walk-off-slab-teleports-to-ground]] → [[research: Sims build-mode edit analog]] → [[feat: Edit hold-drag brush]] → [[feat: Edit paint clicked edge]] → [[research: Event node graph vs bytecode]] → [[walk-through-ladder]] → [[feat: Event graph model and compile]] → [[feat: Event graph editor canvas]]
