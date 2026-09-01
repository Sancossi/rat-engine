---
type: bug
area: Engine
status: Investigating
severity: Medium
sprint: Sprint 7
tags: [bug]
---

# Jump button does not always fire

Origin: chat 2026-09-01 (play). Tick path: [[feat: SimulationSession unified tick]].

## Repro

1. Play `grey_yard` (Space = jump).
2. Press jump while walking, after landing, or with ImGui focused / unfocused.

## Expected

A grounded (or coyote) press always launches; jump buffer covers presses slightly before landing.

## Actual

Jump sometimes does nothing.

## Notes

Suspect leftover from unified tick: `WantCaptureKeyboard` / `clear_pending_input` vs `jump_buffer_left`, or catch-up consuming `jump_pressed` only on the first drain tick. Headless `[sim]` stayed green.

Follow-up from: [[feat: SimulationSession unified tick]]
