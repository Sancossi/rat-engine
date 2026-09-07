---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 12
due:
tags: [task]
---

# chore: Drop event page commands

Intent: после graph-walker `commands[]` не должны быть Play truth и не обязаны жить в save.

Acceptance: dump/save не пишет `commands` (или пустой массив, schema: deprecated). Apply не компилирует graph → list для Play. `docs/schemas/map-event.schema.md` обновлён. Карты `data/maps/` по возможности graph-only. Loader всё ещё принимает legacy commands через `commands_to_graph`.

Depends: [[feat-play-walks-event-graph|feat: Play walks event graph]]. Origin: [[Sprint 12 — Graph is Play truth]]. Related: [[ADR-014 Play executes event graphs]].

## Resolution

`dump_page` не пишет `commands`, если есть `graph`. Apply только валидирует граф. `grey_yard.json` — graph-only. Loader принимает legacy list. Verify: `.\build\tests\rat_tests.exe "[event],[quest],[route],[graph],[map]"`. Review: Approved.

## Bugs found

none.
