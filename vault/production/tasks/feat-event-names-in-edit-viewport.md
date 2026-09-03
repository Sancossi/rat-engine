---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Event names in edit viewport

Intent: в Edit над маркерами ивентов нет имён — в Events submode нужно показывать `EventDef.id`.

Acceptance: `project_world_to_pixels` + ImGui overlay над столбиком маркера (~1.4). Только `AppMode::Edit` + Events submode. Play без подписей. Не текстовые меши в greybox. Unit-тест проекции.

Origin: [[feat: Edit submodes terrain objects events]]. Chat 2026-09-03.
