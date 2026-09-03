---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 9
due:
tags: [task]
---

# feat: Save load game file

Intent: `GameState` уже умеет `save_to_memory` / `load_from_memory`. Нужен файловый слот и UI паузы, чтобы сессию `grey_yard` можно было продолжить.

Acceptance: Play — сохранить в файл (switches, variables, inventory, map id, player xyz); загрузить восстанавливает состояние. Headless round-trip. Не save карты Edit.

Origin: [[GDD]] Pause / save; [[S1: Acceptance playthrough checklist]] (no file save UI). Взято в [[Sprint 9 — First playable loop]]. Related: [[S1: GameState switches/vars/inventory/position]].

## Resolution

Play: Escape — пауза; Save/Load слота `saves/slot1.ratsave` через `save_game` / `load_game` (`RATSAVE1` + FileStore). Load восстанавливает switches/vars/items/map id/**xyz** (Y не снапается на height-grid). Чужой `map_id` — ошибка, сессия не меняется. Не пишет карту Edit. Verify: Play `grey_yard`, Esc Save, сдвинуться, Load; `.\build\tests\rat_tests.exe "[sim]"`. Review: Approved (после фикса Y / map_id).

## Bugs found

none.
