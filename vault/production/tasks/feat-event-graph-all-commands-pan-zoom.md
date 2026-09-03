---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Event graph all commands pan zoom

Intent: canvas MVP только 4 kind и без pan/zoom. Нужны все `CommandOp` как ноды и pan (MMB / Alt+LMB) + wheel zoom вокруг курсора.

Acceptance: расширить `EventGraphNode` + parse/dump + schema. Kinds: control_variable, control_self_switch, transfer_player, change_items, play_se, set_move_route (through + route[], не вложенный граф), comment. Compile не сваливает unknown в Comment. Palette в canvas. Не reverse-compile commands→graph. Golden compile-тесты на каждый kind.

Depends: [[feat: Event graph window pages and conditions]]. Origin: chat 2026-09-03. Related: [[feat: Event graph model and compile]].

## Resolution

Все `CommandOp` — ноды графа (`control_variable`, `self_switch`, `transfer_player`, `change_items`, `play_se`, `set_move_route`, `comment`). Canvas: pan (MMB / Alt+LMB), wheel zoom вокруг курсора. Play по-прежнему только `commands[]`. Verify: Event Graph palette + Apply; `.\build\tests\rat_tests.exe "[graph]"`. Review: Approved.

## Bugs found

none.
