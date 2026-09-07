---
type: task
area: Production
status: In review
review: In review
task_type: Chore
sprint: Sprint 15
tags: [task, audit]
---

# Project audit followups

Origin: запрос пользователя 2026-09-07 на подробную ревизию проекта и оценку дополнительных навыков.

Ревизия выполнена: [отчёт и порядок работ](../../../docs/audits/2026-09-07-project-review.md), [исполняемые примеры](../../../docs/audits/2026-09-07-probe.cpp).
Реализация утверждённого плана идёт в [[Sprint 15 — Stabilization]]. Этапы хранения, авторского цикла, runtime, геометрии и полной локальной GUI-приёмки независимо одобрены. Остались заключительные измерения и проверки доставки в [[s15-acceptance]].

Acceptance: выбрать следующий спринт, исправить ссылки vault и правила процесса (A11–A14), добавить проверки и повторяемый build/desktop loop (A15–A16). Реализацию дефектов вести по отдельным карточкам, не закрывать весь список одним непроверяемым срезом.

Follow-up:

- [[editor-discards-unsaved-map|Editor discards unsaved map]]
- [[file-write-truncates-existing-save|File write truncates existing save]]
- [[save-parser-accepts-incomplete-and-nan|Save parser accepts incomplete and NaN]]
- [[map-validation-accepts-nan|Map validation accepts NaN]]
- [[new-event-id-collides-after-restart|New event ID collides after restart]]
- [[transfer-player-map-state-diverges|Transfer Player map state diverges]]
- [[replay-checksum-omits-npc-position|Replay checksum omits NPC position]]
- [[replay-parser-accepts-unsupported-format|Replay parser accepts unsupported format]]
- [[editor-dirty-checkpoint-inaccurate|Editor dirty checkpoint inaccurate]]
- [[event-touch-exposed-but-unsupported|Event touch exposed but unsupported]]

Уже известные задачи не дублируются: [[chore-ramp-voxel-review-polish]], [[chore-edit-voxel-review-polish]], [[chore-occupancy-review-test-polish]], [[chore-bridge-climb-review-test-polish]], [[event-graph-self-pin-drag-stays-armed]].

Исторический отчёт фиксирует состояние до реализации. Текущие результаты: [индекс доказательств A01–A16](../../../docs/audits/2026-09-07-stabilization-evidence.md). Созданы общие проектные навыки сборки, vault и воспроизведения runtime; установлен screenshot. Дополнительные плагины для реализации не потребовались.

## Resolution

В работе: утверждённые исправления реализуются и принимаются отдельными коммитами. Карточка закрывается после завершения [[s15-acceptance]], финального ревью и полной Release-сборки.

## Bugs found

Дополнительные ошибки, найденные GUI-прогонами и ревью, зафиксированы в карточках этапов и индексе доказательств; исправления входят в утверждённый объём стабилизации.
