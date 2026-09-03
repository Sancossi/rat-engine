---
type: task
area: Game
status: Not started
task_type: Feature
sprint: Sprint 11
due:
tags: [task]
---

# feat: Grey yard move route

Intent: Play на `grey_yard` видит ходящее событие. Runtime уже в [[feat: Set Move Route (basic)]].

Acceptance: отдельный ground NPC (не apprentice / foreman / loft_plank) — Parallel page, короткий `route` туда-обратно на свободных клетках. Switches 1/2 и `rusty_cog` не трогает. `loft_plank` остаётся последним с `y: 2.0`. Headless: после N тиков overlay-клетка сдвинулась.

Origin: [[research: Set Move Route]] (content, не engine). Depends: [[feat: Set Move Route (basic)]]. Взято в [[Sprint 11 — Set Move Route]].
