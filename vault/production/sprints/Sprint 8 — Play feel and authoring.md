---
type: sprint
status: Done
dates: 2026-09-02/2026-09-03
goal: Play feel on grey_yard plus Sims-like edit and event node-graph authoring
current: false
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

Эпики (не в очереди pickup): [[feat-sims-like-edit-brush-and-edge-paint|feat: Sims-like edit brush and edge paint]], [[feat-event-node-graph-authoring|feat: Event node graph authoring]]. Баг [[edit-edges-only-one-facing]] закрывается слайсом paint-clicked-edge.

Порядок:

- [[climb-camera-locks-on-far-side]] → [[feat-smooth-camera-turn|feat: Smooth camera turn]] → [[walk-off-slab-teleports-to-ground]] → [[research-sims-build-mode|research: Sims build-mode edit analog]] → [[feat-edit-hold-drag-brush|feat: Edit hold-drag brush]] → [[feat-edit-paint-clicked-edge|feat: Edit paint clicked edge]] → [[research-event-node-graph|research: Event node graph vs bytecode]] → [[walk-through-ladder]] → [[feat-event-graph-model-and-compile|feat: Event graph model and compile]] → [[feat-event-graph-editor-canvas|feat: Event graph editor canvas]] → [[chore-event-graph-join-golden-and-branch-edges|chore: Event graph join golden and branch edges]]

## Итог

DoD выполнен 2026-09-03. Climb-камера за игроком, плавный поворот; сход со 2 этажа — Fall как прыжок; кисть и покраска ребра в Edit; события — optional graph, compile в `commands`, холст MVP. Play-баги: [[walk-off-slab-teleports-to-ground]], [[walk-through-ladder]] — Fixed.

## Bugs found

none новых на закрытии. Follow-up в [[Sprint 9 — First playable loop]]: [[events-match-ground-while-on-slab]].
