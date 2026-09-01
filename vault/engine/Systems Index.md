---
type: note
tags: [engine]
---

# Systems Index

Каталог подсистем. Статус: `planned` / `stub` / `working` / `deprecated`.

Сверка с GPP — [[Game Programming Patterns]].

| System | Layer | Status | Owner / notes |
| --- | --- | --- | --- |
| Core runtime loop | Core | working | Init → update → render → shutdown. Fixed 120 Hz accumulator in `EditorApp` |
| Logging | Core | working | `rat::Logger` + sink (file/`stderr`/memory); no locator. Default `rat.log` or `RAT_LOG_PATH` |
| Window + swapchain | Platform / Render | working | GLFW + bgfx present |
| Input actions | Input | working | `InputBindings` table + keyboard/gamepad adapters → `InputButtons` → `InputFrame`; JSON serialize; GLFW/joystick only in `NativeWindow` |
| Asset loader | Assets | planned | |
| Scene / entities | Scene | stub | [[ADR-012 Entity model EntityId and ComponentStore]]: `EntityId` + `ComponentStore` Transform/Renderable/Collider; player/events not migrated |
| Audio | Platform | stub | `QueuedAudio` + `AudioSink` (null/log); `EditorApp` drains once per display frame |
| Gameplay notify | Core | stub | GPP Observer: `GameplayNotifyBus` subscribe/post (ItemPicked / DialogShown / Landed); `EventRuntime::set_notify`; no locator |
| Player locomotion | Core | stub | GPP State: `locomotion_from` Idle/Walk/Jump/Fall from `JumpState`+`MoveInput`; names are animation contract; jump physics stays in `JumpState` |
| Collision world | Core | working | Fence + terrain side-face bake; cylinder vs segments with max_step_up skip; event overlap is circle vs AABB. Later crates/layers: [[feat: Field physics puzzles]], [[feat: Stacked surfaces caves and basements]]. Not ECS |
| Edit history | Core | working | GPP Command: `EditHistory` blockers/events plus height-grid/ramps/edges; Ctrl+Z/Y in Edit only; `clear()` on hot-apply |
| Edit gizmos | Input / Scene | planned | [[feat: Mouse viewport map edit]] |
| Agent debug dump | Core | working | `write_debug_snapshot` → `rat-debug.json` (F3); log file `rat.log` |

## Как добавлять систему

1. Строка в этой таблице со статусом `planned`.
2. Задача в [[Tasks]] (Area = Engine).
3. После merge — статус `working` + краткая заметка по API.
