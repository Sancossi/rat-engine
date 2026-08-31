---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: why-not ok should match start locks

Intent: `event_why_not_fired` returns `ok` for Autorun/Parallel/PlayerTouch even when `try_start_*` will not start (spent `autorun_lock_`, already inside touch, parallel cap, foreground busy with another event). Review of [[feat: Event why-not-fired]].

Acceptance: `ok` only if this event would actually start this frame given the same predicates as `try_start_*`; tests for spent autorun lock.

Origin: [[feat: Event why-not-fired]]

## Resolution

`why_not_fired` uses the same start gates as `try_start_*`. Existing reason strings unchanged. New: `autorun_lock`, `foreground_busy`, `parallel_limit`, `already_inside`. `ok` means this event would `start_page` this frame (live instance still `already_running`). Verify: `rat_tests.exe "[why]"`. Stays In progress until review Approved.

## Bugs found

none.
