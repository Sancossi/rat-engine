---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Step off ladder onto same-tile floor

Intent: на верхе лестницы (`y_hi`) движение away должно ставить ноги на пол той же клетки, если там есть support (плита / земля), а не только отпускать в касательный выход за край тайла.

Acceptance: East ladder + slab on the same tile; climb to `top_y`; away → stand on the slab without walking off the tile. Leave-without-support still falls.

Origin: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]] (Task 5 review: face-normal XZ is zeroed while overlapping, so the center cannot stay on the owner tile when leaving). Related: [[feat-ladder-toward-climb-jump-grab|feat: Ladder toward-climb, jump grab, jump off]], [[feat-mgs3-ladder-climb|feat: MGS3 ladder climb]].

Absorbed by [[feat-mgs3-ladder-climb|feat: MGS3 ladder climb]]. Top + up onto same-tile slab shipped there.

## Resolution

Сход сверху: `y_hi` + движение вверх по рельсу ставит ноги на `query_solid_support` после nudge 0.35 face-away (плита той же клетки). Без support — clamp, остаёмся на рельсе. Проверка: `Top of east ladder plus up steps onto the same-tile slab` в `.\build\tests\rat_tests.exe "[unit][player]"`.

## Bugs found

none.
