---
type: task
area: Engine
status: In progress
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Event RMB create edit copy delete

Intent: в Events submode ПКМ по вьюпорту открывает меню Create / Edit / Copy / Delete. Copy ивента сейчас нет.

Acceptance: пусто → Создать на клетке клика (`make_stub_event` + place). По ивенту → Создать / Редактировать (select + открыть граф-окно) / Копировать / Удалить. `make_duplicate_event_command`: уникальный id, tile +1 по X если занято. Undo через EditCommand. Terrain RMB abort brush не ломать. Тесты duplicate в `event_edit_test`.

Depends: [[feat: Edit submodes terrain objects events]]. Origin: chat 2026-09-03. Related: [[feat: Event graph window pages and conditions]].
