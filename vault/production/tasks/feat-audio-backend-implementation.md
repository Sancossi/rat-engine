---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: Audio backend implementation

Intent: заменить log/null sink у [[feat-audio-play-queue-stub|feat: Audio play-queue stub]] реальной реализацией по [[research-audio-backend-adr|research: Audio backend ADR]]. Клипы по `AssetId`. Очередь ограничена; метрика переполнения. Не locator: провайдер по-прежнему composition root. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: PlaySE слышен в editor; тесты подменяют sink без устройства; overflow не молчит. Без ADR в код бэкенда не класть.

Depends: [[research-audio-backend-adr|research: Audio backend ADR]], [[define-asset-folder-load-stub|feat: Asset registry and first load vertical]]. Next: [[chore-engine-frame-metrics|chore: Engine frame metrics]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).

## Resolution

`QueuedAudio` ограничен 64 слотами; лишние посты увеличивают `overflow_count()`. Editor линкует miniaudio 0.11.25 только в `rat-editor`; `MiniaudioSink` играет клипы по `AssetId` (`data/audio/beep.wav`, grey_yard `sfx/beep`). Init fail → `LogAudioSink`. `rat_tests` без устройства. Verify: `.\build\tests\rat_tests.exe "[audio]"` / `"[asset]"` и `ctest`. Review: Approved.

## Bugs found

none.
