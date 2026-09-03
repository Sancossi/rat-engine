---
type: sprint
status: In progress
dates: 2026-09-03/2026-10-01
goal: Set Move Route — NPC walk on the grid, visible on grey_yard
current: true
tags: [sprint]
---

# Sprint 11 — Set Move Route

Цель: закрыть команду v1 [[research: Set Move Route]] — NPC ходит по сетке без physics-ящиков; на `grey_yard` это видно в Play.

DoD:

- `set_move_route` парсится и тикает: overlay, yield как Wait, коллизия по правилам research
- На `grey_yard` событие ходит (Parallel-патруль или Action-катсцена); switches 1/2 не ломаются
- Greybox marker с `EventDef.y` сидит на плите, не на y=0

Вне скоупа: Event touch, party follow, pathfinding, field physics, scare-blocker, spatial partition / FX pool, successor map.

Порядок:

- [[feat: Set Move Route (basic)]] → [[feat: Grey yard move route]] → [[feat: Event marker uses bind Y]]
