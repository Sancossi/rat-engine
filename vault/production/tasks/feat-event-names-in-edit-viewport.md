---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Event names in edit viewport

Intent: в Edit над маркерами ивентов нет имён — в Events submode нужно показывать `EventDef.id`.

Acceptance: `project_world_to_pixels` + ImGui overlay над столбиком маркера (~1.4). Только `AppMode::Edit` + Events submode. Play без подписей. Не текстовые меши в greybox. Unit-тест проекции.

Origin: [[feat: Edit submodes terrain objects events]]. Chat 2026-09-03.

## Resolution

`project_world_to_pixels` + ImGui overlay рисует `EventDef.id` над столбиком маркера (~y+1.4) только в Edit → Events. Play без подписей, greybox без 3D-текста. Verify: F2, Events, имена над cyan-маркерами; `.\build\tests\rat_tests.exe "[viewport_edit]"`. Review: Approved.

## Bugs found

none.
