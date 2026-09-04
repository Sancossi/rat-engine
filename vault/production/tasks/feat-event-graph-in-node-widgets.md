---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 12
due:
tags: [task]
---

# feat: Event graph in-node widgets

Intent: ноды графа — только kind + caption, поля в панели под canvas. Нужны виджеты **внутри** ноды (текст, radio, checkbox, числа), как в референсе event graph (диалог/звук/ветка на самой карточке).

Acceptance: каждая kind рисует свои ImGui-контролы на теле ноды; высота ноды от содержимого; пины then/else у выбора/ветки на строках опций. Отдельный inspector list команд не нужен. Pan/zoom сохраняется. Не imgui-node-editor.

Origin: chat 2026-09-04 (скрин референса). Depends: [[feat: Play walks event graph]]. Origin: [[Sprint 12 — Graph is Play truth]]. Related: [[feat: Event graph all commands pan zoom]].
