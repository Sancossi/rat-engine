---
type: task
area: Engine
status: Not started
task_type: Research
sprint: Sprint 4
due:
tags: [task]
---

# research: Rat debug loopback MCP

Intent: тонкий debug-порт у `rat-editor` (HTTP localhost или named pipe): snapshot, why-not, хвост лога, опционально скрин HWND — по образцу Godot MCP `run_project` / `get_debug_output` и Blender `get_viewport_screenshot`. Общий browser/Godot MCP окно GLFW не видит. Сначала файловый dump ([[feat: Debug snapshot JSON]]); MCP — обёртка. Контекст: [[Agent Debug]].

Acceptance: короткая заметка с выбором (только файлы vs loopback + MCP) и 4–6 методов API; установку чужих MCP в Cursor не делать в этой карточке.
