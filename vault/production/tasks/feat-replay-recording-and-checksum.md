---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: Replay recording and checksum

Intent: воспроизводимость поверх [[feat: SimulationSession unified tick]]. `ReplayHeader` / `TickInput` / `ReplayRecording`; писать `InputFrame` + `tick_id`; явный seed; checksum runtime-состояния; debug snapshot включает tick/input/checksum; при расхождении — первый несовпавший tick. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: повтор одной записи даёт тот же checksum в editor и headless; тест покрывает загрузку карты, движение и одну event-команду.

Depends: [[feat: SimulationSession unified tick]]. Next: [[feat: MapDocument and RuntimeMap compile]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
