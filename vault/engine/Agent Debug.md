---
type: note
tags: [engine]
---

# Agent Debug

Как отлаживать runtime так, чтобы **человек и нейросеть** видели одно и то же без обязательного скриншота. Скрин окна — запасной канал: ImGui и dbgText агент читает плохо, координаты с картинки врёт.

Сейчас: позиция игрока в `bgfx::dbgText`, `EventRuntime::warnings()` только на лимиты Parallel, inspector событий — статический summary, механика проверяется Catch2 headless. Нет лог-файла, нет JSON-снимка кадра, нет «почему этот event не стартанул».

## Принципы

- Текст важнее пикселей: JSON/лог на диск, `stderr` тестов, dump по горячей клавише.
- Причина важнее симптома: не «не сработало», а `height_mismatch` / `conditions` / `not_overlapping` / `input_blocked`.
- Репро без окна: последовательность `InputFrame` + фиксированный dt → тот же `GameState`.
- Не дублировать Unity Profiler. Сначала dump + why-not + headless probe.

## Уже не это

Визуальный profiler, RenderDoc, ECS, мышь в Edit — другие карточки. Логирование как примитив — [[Logging and assert helpers]] (Sprint 4; дописать, не клонировать).

## Sprint 4

1. Наполнить [[Logging and assert helpers]] (файл + уровни, без синглтона).
2. [[feat: Debug snapshot JSON]]
3. [[feat: Event why-not-fired]]
4. [[feat: Headless input sequence probe]]

## MCP и внешние тулзы

Сначала файлы на диске (агент уже умеет `Read` / `Shell`). MCP — обёртка, не замена dump. Выбор зафиксирован в [[Rat debug loopback MCP]]: пока `rat-debug.json` / `rat.log` / probe; loopback не ставим.

Подключено сейчас и **не** закрывает debug GLFW-окна: Notion (для этого репо запрещён), browser MCP (веб, не `rat-editor`), Godot MCP (чужой движок; ориентир API: run / debug output / screenshot), Blender MCP (арт позже; ориентир: `get_viewport_screenshot`).

Имеет смысл по мере надобности:

| Зачем | Что | Когда |
| --- | --- | --- |
| Текст состояния | JSON snapshot + лог-файл | Sprint 4 |
| Как Godot `get_debug_output` | loopback позже (snapshot / why-not / log_tail) | [[Rat debug loopback MCP]] |
| Картинка окна | скрин HWND `rat-editor` (не браузер) | тот же research; capture MCP не ставить, пока dump хватает |
| Сборка / тесты | уже есть: `ctest`, `compile_commands.json`, терминал Cursor | не нужен отдельный MCP |
| Доки bgfx/GLFW | Context7 / docs MCP | nice-to-have, не блокер |

Не тащить: Playwright на native-окно, Discord MCP для багов, Godot MCP «чтобы было».

