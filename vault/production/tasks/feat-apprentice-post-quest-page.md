---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 13
due:
tags: [task]
---

# feat: Apprentice post-quest page

Intent: apprentice на `grey_yard` всегда говорит, что foreman ещё ищет cog. После switch 2 реплика устаревает.

Acceptance: вторая Action-page при switch 2 (echo «Yard's quiet» / двор уже закрыт); до turn-in первая page без изменений. Не трогать switches 1–2 и `rusty_cog`.

Origin: [[feat-grey-yard-apprentice-npc|feat: Grey yard apprentice NPC]] (review Minor). Origin: [[Sprint 13 — Grey yard map pass]].

## Resolution

Вторая Action-page у `apprentice` при switch 2: «Ученик: Двор уже закрыт. Во дворе пока тихо. Хорошая работа.» Первая page до turn-in без изменений. Switches 1–2 и `rusty_cog` не трогали. Review: Approved.

Verify: сдать cog мастеру, снова поговорить с учеником. `.\build\tests\rat_tests.exe "[quest]"`.

## Bugs found

none.
