---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Event graph model and compile

Intent: модель графа (ноды + рёбра) и compile → существующий `EventPage` / `Command`. Play не читает граф. Headless round-trip JSON.

Acceptance: serialize/load graph; compile matches today’s linear list for a golden page; invalid graph = structured error, карта не стартует. Нет ImGui canvas.

Origin: [[feat: Event node graph authoring]]. Depends: [[research: Event node graph vs bytecode]]. Взято в [[Sprint 8 — Play feel and authoring]].

Follow-up: [[feat: Event graph editor canvas]] — compile-on-apply; join-after-branch golden; sequence edges on `conditional_branch` should error.

## Resolution

Optional `EventPage::graph` round-trips in map JSON. `compile_event_graph` walks `entry` → MVP nodes (Show Text, Switch, Branch, Wait) into `commands[]`. Play / `EventRuntime` still execute `commands` only. Invalid graph → `MapIssue`; validate/load reject. Verify: `.\build\tests\rat_tests.exe "[event],[map]"`. Review: Approved.

## Bugs found

none.
