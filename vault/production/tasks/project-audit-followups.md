---
type: task
area: Production
status: Done
review: Approved
task_type: Chore
sprint: Sprint 15
tags: [task, audit]
---

# Project audit followups

Origin: запрос пользователя 2026-09-07 на подробную ревизию проекта и оценку дополнительных навыков.

Ревизия выполнена: [отчёт и порядок работ](../../../docs/archive/cpp/audits/2026-09-07-project-review.md), [исполняемые примеры](https://github.com/sancossi/rat-engine/blob/60fdc55f19889304fc29957fd2124624077aefcd/docs/audits/2026-09-07-probe.cpp).
Утверждённый план реализован в [[Sprint 15 — Stabilization]]. Все этапы, включая измерения и локальные проверки доставки в [[s15-acceptance]], независимо одобрены.

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

Исторический отчёт фиксирует состояние до реализации. Текущие результаты: [индекс доказательств A01–A16](../../../docs/archive/cpp/audits/2026-09-07-stabilization-evidence.md). Созданы общие проектные навыки сборки, vault и воспроизведения runtime; установлен screenshot. Дополнительные плагины для реализации не потребовались.

## Resolution

Завершено: A01–A16, четыре включённых follow-up Sprint 14 и ошибка self-pin исправлены и проверены отдельными срезами. Release и свежий headless прошли по 707 тестов; реальный GUI и установленный ZIP — по 40 сценариев, clang-tidy — 45/45, Python — 10/10; vault, actionlint и benchmark прошли. Полная Release-сборка выполнена. Удалённые CI, Linux и ASan/UBSan настроены, но в этой среде не запускались. Полный перечень доказательств и ограничений — в связанном индексе A01–A16.

## Bugs found

Дополнительные ошибки, найденные GUI-прогонами и ревью, зафиксированы в карточках этапов и индексе доказательств; исправления входят в утверждённый объём стабилизации.
