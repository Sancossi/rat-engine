---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Debug snapshot JSON

Intent: снимок кадра симуляции в JSON-файл (F3 или API), чтобы нейросеть читала состояние без скриншота. Поля: sim frame, `AppMode`, player xyz + `JumpState`, overlapping event ids, active interpreter (event/page/index/wait), `GameState` switches/vars/items, `active_message`, `warnings`. Контекст: [[Agent Debug]].

Acceptance: `write_debug_snapshot(path, ...)` в `rat_core`; editor пишет по F3; round-trip тест на фикстуре без окна.

## Resolution

`make_debug_snapshot` / `write_debug_snapshot` / `read_debug_snapshot`. Файл по умолчанию `rat-debug.json`. F3 в editor. Поля: sim_frame, app_mode, player, jump, overlapping ids, interpreter, switches/vars/items, message, warnings. Тесты: `[debug]`.

## Bugs found

none на момент закрытия. Why-not в снимок добавлен следующей карточкой.
