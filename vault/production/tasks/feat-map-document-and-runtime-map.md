---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: MapDocument and RuntimeMap compile

Intent: убрать authoring из `EventRuntime`. `MapDocument` владеет terrain, ramps, blockers, edge barriers и event definitions. Pipeline: JSON → schema → migration → semantic validation → `MapDocument` → compile → неизменяемый на tick `RuntimeMap`. Ревизии документа; структурированные ошибки (severity, JSON path). Undo высот/рёбер уже есть ([[feat-undo-height-and-edge-edits|feat: Undo height-grid and edge edits]]) — расширять `EditHistory` только если дырка осталась. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: `EventRuntime` не меняет карту; битая карта не стартует симуляцию; undo/redo даёт семантически тот же `MapDocument`; save/load round-trip.

Depends: [[feat-replay-recording-and-checksum|feat: Replay recording and checksum]]. Next: [[jump-button-does-not-always-fire]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).

## Resolution

`MapDocument` wraps `MapData` + revision; compile validates then copies to `RuntimeMap`. `EventRuntime` no longer mutates the map. `load(MapData)` returns `[[nodiscard]] MapCompileResult` (no silent no-op). Height-grid fallback only for schema v1. Verify: `.\build\tests\rat_tests.exe "[mapdoc]"` and `ctest`. Review: Approved after load/fallback fix.

## Bugs found

none.

