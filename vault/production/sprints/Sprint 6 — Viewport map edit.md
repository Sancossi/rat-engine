---
type: sprint
status: Done
dates: 2026-09-29/2026-10-12
goal: Mouse viewport authoring plus undo for height-grid and edge fences
current: false
tags: [sprint]
---

# Sprint 6 — Viewport map edit

Цель: в Edit править карту мышью во вьюпорте, а не только ImGui; правки height-grid и рёбер обратимы.

DoD:

- клик выбирает ближайший event/blocker; drag по XZ земли; клик по пустой клетке ставит объект активного инструмента; `WantCaptureMouse` не крадёт клики
- Place cube / ramp / edge fence пишут `EditHistory`; Ctrl+Z/Y в Edit; Play стек не пишет
- мышью на клетке: Place cube и пресет ребра (Mini/Full/Remove); greybox и inspector синхронизируются

Вне скоупа: vertical slice контент, audio ADR, ECS, ребинд/геймпад, object pool, spatial partition.

Порядок:

- [[feat-mouse-viewport-map-edit|feat: Mouse viewport map edit]] → [[feat-undo-height-and-edge-edits|feat: Undo height-grid and edge edits]] → [[edge-walls-hang-in-the-air]] → [[feat-mouse-viewport-terrain-edit|feat: Mouse viewport terrain edit]] → [[edge-walls-passable-from-adjacent-side]] → [[feat-bake-height-grid-and-ramps-to-collision-solids|feat: Bake height-grid and ramps to collision solids]]

## Итог

DoD выполнен 2026-09-01. Мышь: select/drag/place, `WantCaptureMouse`. Undo height-grid, ramps и edge fences через `EditHistory`. Viewport Place cube и пресеты ребра. Greybox заборы на углах клетки, боковые грани height-grid до Y=0. Cylinder vs baked fence segments; side faces куба в `CollisionWorld`. Архитектурный срез не брали → [[Sprint 7 — Engine architecture]].

## Bugs found

none новых на закрытии. Follow-up collision: [[edge-walls-passable-from-adjacent-side]], [[feat-bake-height-grid-and-ramps-to-collision-solids|feat: Bake height-grid and ramps to collision solids]] — Fixed/Done в этом спринте.
