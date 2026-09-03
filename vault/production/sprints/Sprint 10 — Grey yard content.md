---
type: sprint
status: Done
dates: 2026-09-03/2026-09-17
goal: Grey yard content — loft-placed events, a third NPC, art moodboard lock
current: false
tags: [sprint]
---

# Sprint 10 — Grey yard content

Цель: наполнить зафиксированную сессию ([[Define vertical slice scope]]) тем, чего не хватает GDD на той же карте: триггер на плите, 2–3 NPC, визуальный lock. Не Set Move Route и не field physics.

DoD:

- Событие на клетке лофта fire только стоя на плите; с земли под ней — нет; на `grey_yard` есть такой beat
- Третий NPC на дворе (кроме foreman + loft beat)
- Moodboard: палитры FF6 / Chrono Cross записаны в [[Art Direction]]

Вне скоупа: Set Move Route / NPC walk (только research-заметка), scare-blocker, field physics, spatial partition / FX pool, successor map.

Порядок:

- [[feat: Loft event height]] → [[feat: Grey yard apprentice NPC]] → [[Art direction moodboard pass]] → [[research: Set Move Route]]

## Итог

DoD выполнен 2026-09-03. `loft_plank` на (0,4) с `y: 2.0`; третий голос `apprentice` (−3, 1); Moodboard — 7 hex в [[Art Direction]]. Research: overlay / `route[]` / один interpreter; impl — [[feat: Set Move Route (basic)]].

## Bugs found

none новых на закрытии. Backlog: [[feat: Event marker uses bind Y]], [[feat: Apprentice post-quest page]], [[chore: Named grey_yard look-refs]], [[feat: Set Move Route (basic)]].
