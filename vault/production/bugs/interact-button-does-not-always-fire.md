---
type: bug
area: Engine
status: Investigating
severity: Medium
sprint: Sprint 7
tags: [bug]
---

# Interact button does not always fire

Origin: chat 2026-09-01 (play). Tick path: [[feat: SimulationSession unified tick]]. Sibling: [[jump-button-does-not-always-fire]].

## Repro

1. Play `grey_yard` (E = interact).
2. Press interact at an NPC / scrap / dialog continue, while walking, or with ImGui focused / unfocused.

## Expected

A press in range always starts the action (or continues dialog); interact buffer covers presses slightly before range.

## Actual

Interact sometimes does nothing.

## Notes

Jump already latches a display-frame edge when catch-up runs 0 sim-ticks (`note_jump_pressed`). Interact is still a one-frame `interact_pressed` edge: `drain_simulation_catch_up` clears it after the first tick and does not latch it when `to_run == 0`. Headless `[sim]` can stay green.

Follow-up from: [[feat: SimulationSession unified tick]]
