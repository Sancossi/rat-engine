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
| Input actions | Input | working | `InputFrame` via `map_input_frame`; GLFW only in editor adapter |
| Asset loader | Assets | planned | |
| Scene / entities | Scene | planned | [[research: Entity model ECS vs scene vs hybrid]] |
| Audio | Platform | stub | `QueuedAudio` + `AudioSink` (null/log); `EditorApp` drains once per display frame |
| Gameplay notify | Core | stub | GPP Observer: `GameplayNotifyBus` subscribe/post (ItemPicked / DialogShown / Landed); `EventRuntime::set_notify`; no locator |
| Edit gizmos | Input / Scene | planned | [[feat: Mouse viewport map edit]] |
| Agent debug dump | Core | working | `write_debug_snapshot` → `rat-debug.json` (F3); log file `rat.log` |

## Как добавлять систему

1. Строка в этой таблице со статусом `planned`.
2. Задача в [[Tasks]] (Area = Engine).
3. После merge — статус `working` + краткая заметка по API.
