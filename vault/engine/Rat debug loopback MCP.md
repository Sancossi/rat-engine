---
type: note
tags: [engine]
---

# Rat debug loopback MCP

Исследование, не ADR. DoD Sprint 4: заметка; порт, HTTP, named pipe и MCP в Cursor **не** ставим.

Контекст: [[Agent Debug]]. Уже в `rat_core`: `write_debug_snapshot`, `event_why_not_fired`, `rat.log` / `RAT_LOG_PATH`, `run_input_sequence`.

## Выбор

**Рекомендация: остаёмся на файлах** (`rat-debug.json`, `rat.log`, Catch2 probe). Loopback + MCP — позже, когда dump станет узким местом live-сессии.

Почему:

- Агент уже читает диск (`Read` / `Shell`). F3 пишет `rat-debug.json`; лог — `rat.log` или `RAT_LOG_PATH`; why-not лежит в снимке; репро без окна — `run_input_sequence` в тестах.
- HTTP или named pipe в `rat-editor` требует живой процесс, bind, конфиг Cursor и не помогает CI/headless.
- Образец Godot (`run_project` / `get_debug_output`) имеет смысл, когда агент крутит **открытый** editor и F3/файл мешает. Сейчас dump закрывает принципы Agent Debug.
- Browser/Godot MCP окно GLFW не видят; скрин HWND — отдельная Windows-обёртка, не повод ставить loopback в этом спринте.

Триггер пересмотра: агент итерирует live-editor и файл/F3 тормозит, или нужен `hwnd_screenshot` для визуального бага, который JSON не ловит. Транспорт тогда: localhost HTTP на `127.0.0.1` (проще `curl`, как Godot MCP), не named pipe.

## Методы API (если loopback позже)

Тонкая обёртка над `rat_core`, не новый runtime. Имена — черновик контракта.

| Метод | Контракт |
| --- | --- |
| `snapshot` | Текущий `DebugSnapshot` JSON (тот же контракт, что F3 / `write_debug_snapshot` → `rat-debug.json`). |
| `why_not(event_id)` | `event_why_not_fired` → `wrong_page` / `conditions` / `height` / `not_overlapping` / `out_of_action_range` / `input_blocked` / `already_running` / `autorun_lock` / `foreground_busy` / `parallel_limit` / `already_inside` / `ok`. |
| `log_tail(n)` | Последние N строк `rat.log` (или путь из `RAT_LOG_PATH`). |
| `run_input_sequence` | Headless прогон `InputFrame` шагов → `InputSequenceResult`; опционально snapshot на шаг. GLFW не нужен. |
| `hwnd_screenshot` | PNG окна `rat-editor` (HWND/GLFW), не браузер. Опционально: ImGui/dbgText с картинки агент читает плохо. |
| `warnings` | `EventRuntime::warnings()` (лимиты Parallel и прочее runtime), без полного snapshot. |

## Не делать

- Playwright / browser MCP на native GLFW-окно.
- Discord MCP «для багов».
- Ставить Godot MCP «чтобы было» — чужой движок, окно `rat-editor` не видит.
- Ставить любой Windows-capture MCP до тех пор, пока файловый dump не исчерпан.
- Реализовывать порт или прописывать MCP в Cursor в этой карточке.
