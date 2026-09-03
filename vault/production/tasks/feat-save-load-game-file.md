---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 9
due:
tags: [task]
---

# feat: Save load game file

Intent: `GameState` уже умеет `save_to_memory` / `load_from_memory`. Нужен файловый слот и UI паузы, чтобы сессию `grey_yard` можно было продолжить.

Acceptance: Play — сохранить в файл (switches, variables, inventory, map id, player xyz); загрузить восстанавливает состояние. Headless round-trip. Не save карты Edit.

Origin: [[GDD]] Pause / save; [[S1: Acceptance playthrough checklist]] (no file save UI). Взято в [[Sprint 9 — First playable loop]]. Related: [[S1: GameState switches/vars/inventory/position]].
