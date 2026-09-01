---
type: sprint
status: Active
dates: 2026-09-29/2026-10-12
goal: Mouse viewport authoring plus undo for height-grid and edge fences
current: true
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

- [[feat: Mouse viewport map edit]] → [[feat: Undo height-grid and edge edits]] → [[edge-walls-hang-in-the-air]] → [[feat: Mouse viewport terrain edit]] → [[edge-walls-passable-from-adjacent-side]] → [[feat: Bake height-grid and ramps to collision solids]]
