---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 13
due:
tags: [task]
---

# feat: Grey yard Russian texts

Intent: кириллица в UI уже есть, а двор всё ещё на английском. Нужны русские `show_text` на `grey_yard` (intro, NPC, scrap, crates, loft, gantry).

Acceptance: все `show_text` в `data/maps/grey_yard.json` — русский UTF-8; смысл квеста (cog / crates / winch) тот же. Не переводить chrome редактора. Не i18n-система. Тесты, которые матчят английские строки, обновить; `[quest]` по switches/items не ломать.

Depends: [[feat: Cyrillic in texts and comments]]. Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[feat: Grey yard layout pass]].

## Resolution

Все 12 `show_text` на `grey_yard` — русский UTF-8 (Мастер / Ученик; шестерня, ящики, лебёдка). Item id `rusty_cog` и switches без изменений. Тест `[unit][map]` на кириллицу; `[quest]` матчит «Ученик». Review: Approved.

Verify: Play — intro и мастер на русском. `.\build\tests\rat_tests.exe "[quest],[smoke],[map]"`.

## Bugs found

none.
