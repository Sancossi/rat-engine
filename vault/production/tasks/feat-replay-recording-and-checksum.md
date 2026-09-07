---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: Replay recording and checksum

Intent: воспроизводимость поверх [[feat-simulation-session-unified-tick|feat: SimulationSession unified tick]]. `ReplayHeader` / `TickInput` / `ReplayRecording`; писать `InputFrame` + `tick_id`; явный seed; checksum runtime-состояния; debug snapshot включает tick/input/checksum; при расхождении — первый несовпавший tick. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: повтор одной записи даёт тот же checksum в editor и headless; тест покрывает загрузку карты, движение и одну event-команду.

Depends: [[feat-simulation-session-unified-tick|feat: SimulationSession unified tick]]. Next: [[feat-map-document-and-runtime-map|feat: MapDocument and RuntimeMap compile]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).

## Resolution

`ReplayHeader` / `TickInput` / `ReplayRecording` поверх `SimulationSession::tick`. Seed в хедере, FNV checksum runtime, snapshot F3/headless с tick/input/checksum, `first_diverging_tick` при мутации входа. Verify: `.\build\tests\rat_tests.exe "[replay]"` и `ctest`. Review: Approved.

## Bugs found

none (Minor: interact buffer не в хеше — не заводил отдельный баг).

