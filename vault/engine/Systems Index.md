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
| Window + swapchain | Platform / Render | working | GLFW + bgfx present |
| Input actions | Input | planned | [[feat: Input action mapping]] |
| Asset loader | Assets | planned | |
| Scene / entities | Scene | planned | [[research: Entity model ECS vs scene vs hybrid]] |
| Audio | Platform | planned | [[feat: Audio play-queue stub]] |
| Edit gizmos | Input / Scene | planned | [[feat: Mouse viewport map edit]] |
| Agent debug dump | Core | planned | [[Agent Debug]] — snapshot / why-not / headless probe |

## Как добавлять систему

1. Строка в этой таблице со статусом `planned`.
2. Задача в [[Tasks]] (Area = Engine).
3. После merge — статус `working` + краткая заметка по API.
