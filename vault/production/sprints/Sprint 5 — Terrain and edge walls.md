---
type: sprint
status: Done
dates: 2026-09-15/2026-09-28
goal: Height-grid standable cubes and edge fences, visible in greybox and editable in Edit
current: false
tags: [sprint]
---

# Sprint 5 — Terrain and edge walls

Цель: height-grid даёт standable кубы и заборы на рёбрах; это видно в greybox и правится в Edit.

DoD:

- клетка — куб: сбоку не пройти выше `max_step_up`, сверху стоять; greybox рисует боковые грани; инструмент «поставить куб» (+1.0, не ramp)
- `edge_barriers` в schema v2: мини 0.45 перепрыгивается, полная 1.6 — нет даже с hold; ребро не support
- ImGui N/E/S/W + пресеты; на `grey_yard` есть куб, мини-ребро и полное; save/load roundtrip

Вне скоупа: мышь во вьюпорте, undo высот/рёбер, vertical slice, audio ADR, ECS.

Порядок:

- [[feat: Terrain wall cubes]] → [[feat: Grid edge barriers]] → [[feat: Edit and greybox edge walls]]

## Итог

DoD выполнен 2026-09-01. [[feat: Terrain wall cubes]]: боковые грани + Place cube +1.0. [[feat: Grid edge barriers]]: schema v2 `edge_barriers`, ходьба режется, 0.45 перепрыгивается, 1.6 нет. [[feat: Edit and greybox edge walls]]: ImGui N/E/S/W Mini/Full, greybox, `grey_yard` куб + оба забора. Мышь, undo высот/рёбер, slice — не брали → [[Sprint 6 — Viewport map edit]].
