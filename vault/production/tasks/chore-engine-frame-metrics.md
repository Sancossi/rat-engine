---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: Engine frame metrics

Intent: этап 9 roadmap — сначала измерения. Метрики: длительность simulation tick, event commands/tick, draw calls, transient allocations, asset uploads, audio queue depth, collision candidates. Spatial partition, object pool, SoA, dirty flags, jobs — не в этой карточке ([[feat-spatial-partition-broadphase|feat: Spatial partition broadphase]], [[feat-object-pool-fx|feat: Object pool for short-lived FX]] остаются в бэклоге). Взято в [[Sprint 7 — Engine architecture]].

Acceptance: цифры доступны в debug snapshot или логе без скриншота; порог «берём оптимизацию» записан в карточке follow-up, не в догадке.

Depends: [[feat-audio-backend-implementation|feat: Audio backend implementation]]. Next: none (конец очереди Sprint 7).

Origin: merged `docs/archive/cpp/architecture-roadmap.md` (2026-09-01).

## Resolution

`DebugSnapshot` JSON содержит объект `metrics`: длительность `SimulationSession::tick`, команды событий за тик, число пакетов `RenderWorld`, transient allocator, GPU uploads последнего `pump_loads`, глубина/overflow `QueuedAudio`, размер baked collision. Editor пишет `rat-debug.json` через `collect_frame_metrics`. Verify: `.\build\tests\rat_tests.exe "[metrics]"` и `ctest`. Review: Approved.

## Bugs found

none.
