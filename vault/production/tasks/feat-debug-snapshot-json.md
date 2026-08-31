---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Debug snapshot JSON

Intent: снимок кадра симуляции в JSON-файл (F3 или API), чтобы нейросеть читала состояние без скриншота. Поля: sim frame, `AppMode`, player xyz + `JumpState`, overlapping event ids, active interpreter (event/page/index/wait), `GameState` switches/vars/items, `active_message`, `warnings`. Контекст: [[Agent Debug]].

Acceptance: `write_debug_snapshot(path, ...)` в `rat_core`; editor пишет по F3; round-trip тест на фикстуре без окна.
