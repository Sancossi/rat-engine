---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: Audio backend implementation

Intent: заменить log/null sink у [[feat: Audio play-queue stub]] реальной реализацией по [[research: Audio backend ADR]]. Клипы по `AssetId`. Очередь ограничена; метрика переполнения. Не locator: провайдер по-прежнему composition root. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: PlaySE слышен в editor; тесты подменяют sink без устройства; overflow не молчит. Без ADR в код бэкенда не класть.

Depends: [[research: Audio backend ADR]], [[feat: Asset registry and first load vertical]]. Next: [[chore: Engine frame metrics]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
