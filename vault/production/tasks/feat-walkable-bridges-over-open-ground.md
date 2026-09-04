---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 13
due:
tags: [task]
---

# feat: Walkable bridges over open ground

Intent: мост над двором — сверху идёшь, **снизу проходишь** по земле. Куб height-grid до Y=0 этого не даёт (тело клетки сплошное). Нужен airborne пролёт (плита/мост), не пещера и не successor map.

Acceptance:

- Авторинг: пролёт из `floor_slabs` (или тот же тип с tool Place Bridge = slab на выбранном `top_y`) над **открытой** землёй, не над кубом в той же клетке.
- Play: цилиндр 1.6 идёт по верху; по земле под мостом проходит, не упирается в «потолок», если `top_y - thickness` выше головы (~> 1.65).
- Greybox: виден верх и низ пролёта. Не воксели, не field physics, не новый renderer.

Origin: chat 2026-09-04 (Sprint 13, мосты «пройти под терейном»). Related: [[feat: Stacked surfaces caves and basements]], [[feat: Grey yard layout pass]]. Origin: [[Sprint 13 — Grey yard map pass]].

## Resolution

## Bugs found
