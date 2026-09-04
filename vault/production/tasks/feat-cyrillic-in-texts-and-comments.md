---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 12
due:
tags: [task]
---

# feat: Cyrillic in texts and comments

Intent: в Show Text / Comment (граф, JSON, диалог Play) кириллица должна набираться, сохраняться и рисоваться, а не квадратами. Сейчас строки в UTF-8, глифов в дефолтном ImGui-шрифте нет.

Acceptance:

- ImGui грузит TTF с латиницей + кириллицей **до** `imgui_bgfx::init` (атлас). Путь: `data/fonts/` + OFL/лицензия рядом. Нет файла — лог и fallback на Proggy, без краша.
- `InputText` / multiline на нодах show_text и comment, диалог Play (`TextWrapped` active message) показывают кириллицу.
- Save/load: `serialize_map_to_string` ↔ parse сохраняет `"Привет"` в `node.text` (не ломает JSON). Dump читаемый UTF-8, не обязательно `\uXXXX`.
- Не переводить grey_yard. Не i18n UI редактора.

Origin: chat 2026-09-04. Origin: [[Sprint 12 — Graph is Play truth]]. Related: [[feat: Event graph in-node widgets]].

## Resolution

ImGui грузит `data/fonts/NotoSans-Regular.ttf` (OFL) с `GetGlyphRangesCyrillic` до `imgui_bgfx::init`. Show Text / Comment и диалог Play рисуют кириллицу. JSON roundtrip UTF-8. Нет файла — Proggy. Verify: набрать «Привет» в ноде, Play; `.\build\tests\rat_tests.exe "[map],[graph],[event]"`. Review: Approved.

## Bugs found

none.