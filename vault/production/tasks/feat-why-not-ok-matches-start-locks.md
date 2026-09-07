---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: why-not ok should match start locks

Intent: `event_why_not_fired` returns `ok` for Autorun/Parallel/PlayerTouch even when `try_start_*` will not start (spent `autorun_lock_`, already inside touch, parallel cap, foreground busy with another event). Review of [[feat-event-why-not-fired|feat: Event why-not-fired]].

Acceptance: `ok` only if this event would actually start this frame given the same predicates as `try_start_*`; tests for spent autorun lock.

Origin: [[feat-event-why-not-fired|feat: Event why-not-fired]]

## Resolution

`why_not_fired` uses the same start gates as `try_start_*`. Existing reason strings unchanged. New: `autorun_lock`, `foreground_busy`, `parallel_limit`, `already_inside`. `ok` means this event would `start_page` this frame (live instance still `already_running`). Verify: `rat_tests.exe "[why]"`.

Follow-up: [[chore-why-not-start-lock-review-polish|chore: why-not start-lock review polish]]
Follow-up: [[feat-debug-snapshot-why-not-for-selected-event|feat: Debug snapshot why-not for selected event]]

## Bugs found

none (продуктовых). Minor ревью: нет positive Ok для Parallel/Touch; вики MCP со старым списком кодов — см. follow-up.
