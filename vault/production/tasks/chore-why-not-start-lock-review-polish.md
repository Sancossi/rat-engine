---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: why-not start-lock review polish

Intent: minor из ревью [[feat: why-not ok should match start locks]] (Approved): `[why]` с positive `ok` для Parallel и rising-edge PlayerTouch; таблица кодов в [[Rat debug loopback MCP]] включает `autorun_lock` / `foreground_busy` / `parallel_limit` / `already_inside`.

Acceptance: тесты падают, если Parallel/Touch никогда не `ok`; wiki-таблица совпадает с `event_why_not_name`.

Origin: [[feat: why-not ok should match start locks]]

## Resolution

`[why]` now requires `ok` for a startable Parallel under the cap (before `update`) and for PlayerTouch on the rising edge (overlapping, before the first `update`). Wiki `why_not(event_id)` lists `autorun_lock` / `foreground_busy` / `parallel_limit` / `already_inside` with the existing codes. Start-lock predicates unchanged. Review: Approved.

Verify: `.\build\tests\rat_tests.exe "[why]"`.

## Bugs found

none.
