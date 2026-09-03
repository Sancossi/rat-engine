---
type: sprint
status: Done
dates: 2026-09-03/2026-09-16
goal: First playable loop — stacked-floor events, persist a session, inventory UI, lock vertical-slice scope
current: false
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

## Итог

DoD выполнен 2026-09-03. Ground-event не fire с плиты; Esc save/load (`saves/slot1.ratsave`); Play **I** — RM-список; пятёрка референсов; сессия = `grey_yard` ([[Define vertical slice scope]]).

## Bugs found

none новых на закрытии. Backlog: [[feat: Loft event height]].
