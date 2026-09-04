---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 12
due:
tags: [task]
---

# feat: Reverse-compile commands to graph

Intent: карты без `graph` (grey_yard) должны получить граф из `commands[]`, иначе Play не сможет уйти со списка.

Acceptance: `commands_to_graph` в `rat_core`; round-trip `compile_event_graph(commands_to_graph(cmds))` совпадает с cmds для linear, nested branch, wait, set_move_route. Loader: если graph нет — заполняет из commands. JSON файлы карт не обязательно переписывать. Не менять EventRuntime stepper в этом срезе.

Origin: [[Sprint 12 — Graph is Play truth]]. Related: [[ADR-014 Play executes event graphs]]. Follow-up: [[feat: Play walks event graph]].

## Resolution

`commands_to_graph` строит page.graph из списка команд (linear, nested branch, wait, set_move_route). `compile_map_data` / parse_page заполняют graph, если его не было. EventRuntime stepper пока на commands. Verify: `.\build\tests\rat_tests.exe "[graph],[event],[quest],[route]"`. Review: Approved.

## Bugs found

none.

Spec: `docs/superpowers/specs/2026-09-04-play-walks-event-graph-design.md`
