---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Terrain wall cubes

Intent: клетка height-grid как standable куб-стена. Сбоку не пройти, если выше `max_step_up`; сверху можно стоять. Сейчас greybox рисует только верхние квады — куб сбоку не виден.

Acceptance: боковые грани на разрывах `ground_y`; инструмент «поставить куб» поднимает клетку на 1.0 (не на ramp); headless-тест геометрии + height edit.

Depends: none. Next: [[feat: Grid edge barriers]].
