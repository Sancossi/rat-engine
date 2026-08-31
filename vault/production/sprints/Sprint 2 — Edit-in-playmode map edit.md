---
type: sprint
status: Done
dates: 2026-09-01/2026-09-14
goal: "Edit-in-playmode: map + events authoring loop (flat)"
current: false
tags: [sprint]
notion_id: 3cdf3827-36cc-81a7-a028-dfea321ecf15
---

# Sprint 2 — Edit-in-playmode map edit

Цель: править карту и события в playmode без отдельного editor-only пайплайна ([[ADR-006 Edit-in-playmode author loop]]).

DoD:

- Play/Edit hotkey
- Можно двигать blockers/events и hot-apply
- Self-switches работают
- Save/load map JSON stub
- Acceptance: правка → сразу играть квест

Вне скоупа: multi-level / stairs.

## Итог

Sprint закрыт 2026-08-31. Реализованы Play/Edit, hot-apply, редактирование blockers/events, inspector, self-switches и JSON save/load. Acceptance подтверждён: изменения карты применяются и проверяются в игре без отдельного editor-only цикла. Регрессии: 48/48 тестов.
