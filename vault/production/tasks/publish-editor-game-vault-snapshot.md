---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 19
review: Pending
due:
tags: [task, publication, expedition]
---

# Опубликовать редактор, игру и vault после P1.1a

Intent: по запросу пользователя 2026-09-09 сохранить и отправить в `sancossi/rat-engine` завершённый этап камеры вместе с исходниками редактора, обеих реализаций игры и новыми записями vault.

Acceptance:

- Проверен полный состав локальных изменений; прежняя C++ реализация сохранена как история, текущая Stride-игра и закреплённая исходная основа доступны по документации.
- Исходники, оригинальные ассеты, спецификации и vault закоммичены явно выбранными путями. Сборки, зависимости и временные файлы остаются вне Git.
- Независимое read-only ревью состава и проверки сборки/vault записаны в [отчёте публикации](../../../docs/audits/2026-09-09-repository-publication.md).
- Ветка `publish/stride-p11-editor-game-vault` отправлена в существующий origin без переписывания истории; remote HEAD совпадает с локальным.
- Полный P1 и Sprint 19 не закрываются этой публикацией. Чистый upstream Stride остаётся отдельным checkout, его код не дублируется в rat-engine.

Origin: [[expedition-prototype-traversal]]; [[feat-stride-source-foundation]].

## Resolution

В работе. Git push dry-run подтвердил доступ к существующему origin.

## Bugs found

Пока не проверено.
