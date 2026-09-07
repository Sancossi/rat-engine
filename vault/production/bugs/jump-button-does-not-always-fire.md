---
type: bug
area: Engine
status: Fixed
severity: Medium
sprint: Sprint 7
tags: [bug]
---

# Jump button does not always fire

Origin: chat 2026-09-01 (play). Tick path: [[feat-simulation-session-unified-tick|feat: SimulationSession unified tick]].

## Repro

1. Play `grey_yard` (Space = jump).
2. Press jump while walking, after landing, or with ImGui focused / unfocused.

## Expected

A grounded (or coyote) press always launches; jump buffer covers presses slightly before landing.

## Actual

Jump sometimes does nothing.

## Notes

Suspect leftover from unified tick: `WantCaptureKeyboard` / `clear_pending_input` vs `jump_buffer_left`, or catch-up consuming `jump_pressed` only on the first drain tick. Headless `[sim]` stayed green.

Follow-up from: [[feat-simulation-session-unified-tick|feat: SimulationSession unified tick]]

## Resolution

Display-кадр с `jump_pressed` при 0 sim-ticks терял ребро Space (`previous_buttons_` съедал edge до `tick`). `drain_simulation_catch_up` теперь зовёт `note_jump_pressed()` даже при `to_run == 0`. Verify: `.\build\tests\rat_tests.exe "*zero-tick*"` и `[sim]`. Review: Approved.

Follow-up: [[interact-button-does-not-always-fire]] — тот же catch-up edge, E не защёлкнут при `to_run == 0`.

## Bugs found

none.

