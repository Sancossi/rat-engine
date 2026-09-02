---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Step off ladder onto same-tile floor

Intent: на верхе лестницы (`y_hi`) движение away должно ставить ноги на пол той же клетки, если там есть support (плита / земля), а не только отпускать в касательный выход за край тайла.

Acceptance: East ladder + slab on the same tile; climb to `top_y`; away → stand on the slab without walking off the tile. Leave-without-support still falls.

Origin: [[feat: Stacked surfaces caves and basements]] (Task 5 review: face-normal XZ is zeroed while overlapping, so the center cannot stay on the owner tile when leaving). Related: [[feat: Ladder toward-climb, jump grab, jump off]], [[feat: MGS3 ladder climb]].

Absorbed by [[feat: MGS3 ladder climb]] — parent will close after review. Top + up onto same-tile slab shipped in Task 2.
