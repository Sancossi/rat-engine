---
type: task
area: Engine
status: Not started
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
