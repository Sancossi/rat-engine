---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 11
due:
tags: [task]
---

# feat: Grey yard move route

Intent: Play на `grey_yard` видит ходящее событие. Runtime уже в [[feat-set-move-route-basic|feat: Set Move Route (basic)]].

Acceptance: отдельный ground NPC (не apprentice / foreman / loft_plank) — Parallel page, короткий `route` туда-обратно на свободных клетках. Switches 1/2 и `rusty_cog` не трогает. `loft_plank` остаётся последним с `y: 2.0`. Headless: после N тиков overlay-клетка сдвинулась.

Origin: [[research-set-move-route|research: Set Move Route]] (content, не engine). Depends: [[feat-set-move-route-basic|feat: Set Move Route (basic)]]. Взято в [[Sprint 11 — Set Move Route]].

## Resolution

`yard_walker` на (−4, −1), Parallel, east / wait 20 / west / wait 20. `loft_plank` последний с `y: 2.0`. `tile_on_map` сначала height-grid (отрицательные клетки). Verify: Play — cyan southwest of spawn ходит туда-обратно; квест foreman как раньше. `.\build\tests\rat_tests.exe "[route],[quest]"`. Review: Approved.

## Bugs found

none.
