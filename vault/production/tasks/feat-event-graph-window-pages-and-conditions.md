---
type: task
area: Engine
status: In review
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Event graph window pages and conditions

Intent: canvas и pages сидят в Inspector. Нужно dockable окно Event Graph: вкладки = pages, RMB add/copy/delete, блок trigger+conditions вне графа.

Acceptance: окно открывается в Events submode по Редактировать / кнопке панели. Tabs `Page 1…`; последнюю page не удалять; copy page дублирует `EventPage` включая `graph`. Trigger combo + полный AND conditions (Switch/Variable/Item/Self-switch add/remove). Canvas вынести из Inspector; list команд — fallback без `graph`. Не imgui-node-editor. Тесты page CRUD не только ImGui.

Depends: [[feat: Event RMB create edit copy delete]]. Origin: chat 2026-09-03. Related: [[feat: Event graph editor canvas]], [[research: Event node graph vs bytecode]].
