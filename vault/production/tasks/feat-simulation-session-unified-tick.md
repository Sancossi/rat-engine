---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: SimulationSession unified tick

Intent: одна fixed-step симуляция вместо логики, размазанной по `EditorApp` и `input_sequence.cpp`. `SimulationSession` владеет `PlayerBody`, `JumpState`, `GameState`, `EventRuntime`, монотонным `tick_id`. Контракт: `SimulationTickResult tick(const InputFrame&)`. Accumulator и interpolation — снаружи. `run_input_sequence()` — адаптер. Catch-up ticks ограничены. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: editor и headless зовут одну реализацию tick; результат не зависит от display FPS; существующие механик-тесты зелёные; API не принимает platform/render типы.

Depends: [[chore: rat_core isolation from platform graphics]]. Next: [[feat: Replay recording and checksum]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).

## Resolution

`SimulationSession::tick(InputFrame)` in `rat_core` owns player, jump, GameState, EventRuntime, `tick_id`. Editor catch-up and `run_input_sequence` share that tick; accumulator stays in the shell. `WantCaptureKeyboard` zeros `jump_buffer_left` via `clear_pending_input`. Verify: `.\build\tests\rat_tests.exe "[sim]"` and `ctest`. Review: Approved after capture-buffer fix.

## Bugs found

- [[jump-button-does-not-always-fire]] — play 2026-09-01, Space не всегда запускает прыжок.
- [[ramp-climb-does-not-work]] — play 2026-09-01, подъём по рампе не работает.
- [[interact-button-does-not-always-fire]] — play 2026-09-01, E не всегда запускает interact.
- [[cannot-fall-off-ramp]] — play 2026-09-01, нельзя упасть с рампы (через [[ramp-climb-does-not-work]]).

Follow-up: [[jump-button-does-not-always-fire]], [[ramp-climb-does-not-work]], [[interact-button-does-not-always-fire]], [[cannot-fall-off-ramp]]


