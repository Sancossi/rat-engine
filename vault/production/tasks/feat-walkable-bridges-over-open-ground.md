---
type: task
area: Engine
status: Done
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

Origin: chat 2026-09-04 (Sprint 13, мосты «пройти под терейном»). Related: [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]], [[feat-grey-yard-layout-pass|feat: Grey yard layout pass]]. Origin: [[Sprint 13 — Grey yard map pass]].

## Resolution

Мост = существующий `floor_slabs` над открытой землёй (`ground_y ≈ 0`), не куб до Y=0. Place Bridge (и Floor slab upsert) отказывает на клетке-кубе. Цилиндр 1.6 ходит по верху и проходит снизу, если `top_y - thickness > ~1.65` (`cylinder_hits_ceiling` только при пересечении головы с низом плиты). Greybox: верх и низ пролёта. Review: Approved.

Verify: Edit → Terrain → Place bridge над dirt; Play — пройти сверху и под плитой. `.\build\tests\rat_tests.exe "[collision],[player],[viewport_edit],[terrain]"`.

## Bugs found

Playtest: [[cannot-climb-onto-grey-yard-bridge|Cannot climb onto grey_yard bridge]] (на пролёт нет подхода). Follow-up: [[feat-voxel-3d-terrain-and-rotating-ramps|feat: Voxel 3D terrain and rotating ramps]].
