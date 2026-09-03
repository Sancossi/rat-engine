---
type: sprint
status: In progress
dates: 2026-09-03/2026-09-16
goal: First playable loop — stacked-floor events, persist a session, inventory UI, lock vertical-slice scope
current: true
tags: [sprint]
---

# Sprint 9 — First playable loop

Цель: закрыть дыры first playable ([[first-playable-loop]]): событие на земле не срабатывает с плиты, слот сохранения, список инвентаря, зафиксировать 5 референсов и scope vertical slice.

DoD:

- Событие на земле не fire, пока игрок стоит на плите (~Y 2)
- Pause/save пишет `GameState` на диск и грузит обратно (карта, позиция, switches/vars/items)
- Play: RM-like список инвентаря (id / qty / key item)
- Заметки: 5 референс-игр и scope одной короткой сессии на `grey_yard` (или наследнике)

Вне скоупа: Set Move Route / NPC walk, полный MZ palette на холсте, field physics puzzles, moodboard, spatial partition / FX pool.

Порядок:

- [[events-match-ground-while-on-slab]] → [[feat: Save load game file]] → [[feat: Inventory list UI]] → [[collect-5-reference-games]] → [[define-vertical-slice-scope]]
