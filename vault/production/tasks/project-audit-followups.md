---
type: task
area: Production
status: Not started
task_type: Chore
sprint:
tags: [task, audit]
---

# Project audit followups

Origin: запрос пользователя 2026-09-07 на подробную ревизию проекта и оценку дополнительных навыков.

Ревизия выполнена: [отчёт и порядок работ](../../../docs/audits/2026-09-07-project-review.md), [исполняемые примеры](../../../docs/audits/2026-09-07-probe.cpp).
Эта карточка относится к последующим исправлениям; их реализация ещё не начата.

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

Спринт не назначен: current: true отсутствует. Никакие плагины/навыки в ходе ревизии не установлены.
