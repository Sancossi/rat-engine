---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 10
due:
tags: [task]
---

# feat: Grey yard apprentice NPC

Intent: на `grey_yard` третий голос (кроме foreman и loft beat). Ground-level Action NPC: подсказка по двору / cog, без Set Move Route.

Acceptance: Play — поговорить с apprentice; pages не ломают cog-квест (switches 1–2). JSON + headless/quest tests if the intro path needs an extra ack. Не scare-blocker.

Origin: [[Define vertical slice scope]] (GDD 2–3 NPC). Взято в [[Sprint 10 — Grey yard content]]. Depends: loft beat может идти раньше, но эта карточка не требует [[feat: Loft event height]].

Follow-up: [[feat: Apprentice post-quest page]]

## Resolution

Третий голос — JSON Action NPC `apprentice` на (−3, 1), только `show_text`. Switches 1/2 и `rusty_cog` не трогает. `loft_plank` остаётся последним с `y: 2.0`. Verify: Play — подойти west of cyan foreman, E; квест foreman/scrap как раньше. `.\build\tests\rat_tests.exe "[quest]"`. Review: Approved.

## Bugs found

none. Review Minor (flavor after turn-in) → [[feat: Apprentice post-quest page]]. HUD Play по-прежнему называет только cyan foreman — pre-existing copy, не дефект этой карточки.
