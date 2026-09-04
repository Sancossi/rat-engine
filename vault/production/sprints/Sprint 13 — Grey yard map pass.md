---
type: sprint
status: In progress
dates: 2026-09-04/2026-09-18
goal: Grey yard map pass — readable workshop layout, Russian copy, leftover NPC page
current: true
tags: [sprint]
---

# Sprint 13 — Grey yard map pass

Цель: тем же `grey_yard` сделать двор читаемым как мастерская и проходимым на русском. Движок графа уже Play-truth; здесь только карта и тексты. Не вторая JSON-карта.

DoD:

- Двор читается как три зоны: земля (квест cog), лофт, gantry — без наложения «всё в куче»
- Все `show_text` на `grey_yard` на русском; кириллица в Play
- Apprentice после switch 2 не твердит про недобытый cog
- Headless cog e2e / `[quest]` зелёные (тайлы можно сдвинуть, тесты обновить)

Вне скоупа: successor map, field physics, меши, scare-blocker, i18n UI редактора, [[Event graph self-pin drag stays armed]].

Порядок:

- [[feat: Grey yard layout pass]] → [[feat: Grey yard Russian texts]] → [[feat: Apprentice post-quest page]]

Roadmap: [[Content vertical slice]]
