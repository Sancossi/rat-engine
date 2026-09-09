---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 19
review: Approved
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

Follow-up: [[publish-accepted-project-editor-main]].

## Resolution

Опубликован общий снимок в [ветке GitHub](https://github.com/Sancossi/rat-engine/tree/publish/stride-p11-editor-game-vault): исходники редактора, текущей Stride-игры, исторического C++ прототипа, оригинальные ассеты, инженерные документы и vault. Подготовка `af7041d` / `a6d4d96`; независимое ревью состава Approved. Первый push и `git ls-remote` подтвердили совпадение remote и локального SHA `a6d4d96969cc4b39f240a9ca364e11207b47fdb5`; заключительный документационный коммит также отправляется и проверяется.

Полный C++ Release и 24 Python / 720 CTest прошли, один symlink-тест skipped; полный Stride GameStudio Release прошёл с 0 ошибок. Неизменённая игра сохраняет проверенный ZIP с 12 Core и 10 executable сценариями. Сборки/caches исключены из Git. [Полный отчёт](../../../docs/audits/2026-09-09-repository-publication.md). P1.2/P1.3 и Sprint 19 остаются открытыми; публикация не заявляет remote CI или ручной плейтест.

## Bugs found

none — блокеров состава публикации не обнаружено.
