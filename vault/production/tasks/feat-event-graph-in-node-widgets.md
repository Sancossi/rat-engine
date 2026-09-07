---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 12
due:
tags: [task]
---

# feat: Event graph in-node widgets

Intent: ноды графа — только kind + caption, поля в панели под canvas. Нужны виджеты **внутри** ноды (текст, radio, checkbox, числа), как в референсе event graph (диалог/звук/ветка на самой карточке).

Acceptance: каждая kind рисует свои ImGui-контролы на теле ноды; высота ноды от содержимого; пины then/else у выбора/ветки на строках опций. Отдельный inspector list команд не нужен. Pan/zoom сохраняется. Не imgui-node-editor.

Origin: chat 2026-09-04 (скрин референса). Depends: [[feat-play-walks-event-graph|feat: Play walks event graph]]. Origin: [[Sprint 12 — Graph is Play truth]]. Related: [[feat-event-graph-all-commands-pan-zoom|feat: Event graph all commands pan zoom]]. Follow-up: [[feat-event-graph-rmb-copy-delete-and-wiring|feat: Event graph RMB add copy delete and wiring]].

## Resolution

Поля каждой kind рисуются на теле ноды; высота и then/else из `event_graph_node_metrics`. Pan/zoom и RMB-cancel работают над карточками (`ChildWindows`); пины снаружи child. Строки: preview при наборе, commit при deactivate. Verify: Event Graph — колесо над карточкой, клик пина, правка Show Text; `.\build\tests\rat_tests.exe "[graph],[event],[edit]"`. Review: Approved.

## Bugs found

none.
