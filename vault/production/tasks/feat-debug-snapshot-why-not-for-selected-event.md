---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Debug snapshot why-not for selected event

Intent: F3/`make_debug_snapshot` пишет `event_why_not_reason` для **выбранного** event id (как Inspector), а не для первого overlapping. Сейчас список `event_why_not` полный, primary — first overlap / first on map.

Acceptance: `make_debug_snapshot` принимает optional `selected_event_id`; editor передаёт selection; тест на фикстуре с двумя events.

Origin: [[feat: Event why-not-fired]]
