---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: MapDocument and RuntimeMap compile

Intent: убрать authoring из `EventRuntime`. `MapDocument` владеет terrain, ramps, blockers, edge barriers и event definitions. Pipeline: JSON → schema → migration → semantic validation → `MapDocument` → compile → неизменяемый на tick `RuntimeMap`. Ревизии документа; структурированные ошибки (severity, JSON path). Undo высот/рёбер уже есть ([[feat: Undo height-grid and edge edits]]) — расширять `EditHistory` только если дырка осталась. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: `EventRuntime` не меняет карту; битая карта не стартует симуляцию; undo/redo даёт семантически тот же `MapDocument`; save/load round-trip.

Depends: [[feat: Replay recording and checksum]]. Next: [[chore: Decompose EditorApp]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
