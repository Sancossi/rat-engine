---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 9
due:
tags: [task]
---

# feat: Inventory list UI

Intent: предметы уже в `GameState` и `change_items` (квест `rusty_cog`). В Play нужен RM-like список, не сетка Diablo.

Acceptance: открыть инвентарь в Play; видны id / quantity / key item; закрыть той же клавишей. Не экип, не drag-drop. Headless не обязателен для ImGui; состояние читается из `GameState`.

Origin: [[GDD]] inventory. Взято в [[Sprint 9 — First playable loop]]. Related: [[S1: GameState switches/vars/inventory/position]].
