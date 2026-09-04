---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 12
due:
tags: [task]
---

# feat: Event graph RMB add copy delete and wiring

Intent: в графе ивентов неудобно: палитра кнопками, связь клик-пин-клик, нет копирования ноды и хоткеев. Нужен RMB-меню, drag-связь и Ctrl+C/V/X/Z/Y пока фокус на canvas.

Acceptance:

- RMB на пустом canvas: **Add** → подменю нод по разделам (Text / Flow / State / World / Audio — существующие kinds, без новых op). **Copy** (если есть selection). **Delete**.
- RMB на ноде: Copy / Delete этой ноды (Add тоже ок).
- Delete — выбранная нода или ребро (не entry/exit).
- Copy — дубликат ноды со смещением; уникальный id; рёбра копии не цеплять к оригиналу без явного paste-на-месте как отдельная нода.
- Хоткеи при наведении/фокусе canvas (не когда InputText ест клавиши): Delete/Backspace удалить; Ctrl+C copy; Ctrl+V paste; Ctrl+Z undo; Ctrl+Y или Ctrl+Shift+Z redo — через существующий `EditorDocument` undo/redo, не отдельный стек графа.
- Связи: зажать ЛКМ на out-пине и протянуть на in-пин (wire preview), отпустить — `connect_event_graph_nodes`. Клик-пин-клик можно оставить как запасной.

Depends: [[feat: Event graph in-node widgets]]. Origin: [[Sprint 12 — Graph is Play truth]] (chat 2026-09-04). Related: [[feat: Event graph editor canvas]].
