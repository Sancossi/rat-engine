---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 19
review: Approved
baseline_commit: 702cae4
due:
tags: [task, expedition, stride, infrastructure]
---

# Перейти на исходную основу Stride upstream

Intent: Выполнить запрос пользователя о переходе на https://github.com/stride3d/stride с возможностью собственных сборок и доработок движка при необходимости.

Specification: [Исходная основа Stride](../../../docs/stride-source-foundation-spec.md); [[ADR-018 Rat expedition uses Stride]]; [исследование MCP](../../../docs/stride-mcp-integration-research.md).

## Acceptance

- Given доступный upstream и инструменты, when выполнен bootstrap, then отдельная копия C:/5_gamedev/stride содержит закреплённый commit и локальную feature-ветку для доработок; исходный rat-engine сохранён.
- Given повторный запуск, when целевой каталог чужой или имеет незакоммиченные изменения, then bootstrap отказывает без перезаписи, reset или удаления.
- Given выбранный исходный baseline, when выполнена Windows Release-сборка Game Studio, then записаны команда, версия SDK, exit code и фактический путь редактора; состояние LFS проверено.
- Given собственная доработка, when используется описанный порядок, then upstream и патчи разделены коммитами; обновление baseline и собственный выпуск воспроизводимы и сохраняют лицензии.
- Given смена основы, when читаются текущие инструкции проекта, then они ведут на исходный Stride и показывают следующий шаг адаптации P1, не запускают историческую C++ очередь.

Origin: [[research-open-engine-foundation]]; [[ADR-018 Rat expedition uses Stride]].
Follow-up: [[expedition-prototype-traversal]].

## Resolution

Подготовлен официальный Stride в `C:/5_gamedev/stride`, закреплён SHA `e2c786a45f69917bf233793f6a097b150e2fe264`, создана локальная ветка `rat/expedition-foundation`. Добавлены lock, workspace, безопасные bootstrap/build scripts и порядок собственных доработок/выпусков. Реальная Windows x64 Release-сборка Game Studio `4.4.0-dev` успешна; startup smoke достиг окна выбора проекта. Интерактивная сцена, перенос P1 и MCP ещё не проверялись.

Реализация: `8c1dbae`, документация запуска: `673af4a`, исправления ревью: `22b9cf3`. Два независимых read-only reviewer одобрили итог; проверены adversarial, edge-case и acceptance аспекты. Восемь fixture-сценариев passed; повторная сборка исправленным wrapper: exit 0, 0 ошибок, 5 upstream NU5100. [Свидетельства и ограничения](../../../docs/audits/2026-09-08-stride-source-foundation.md), [рабочий процесс](../../../docs/stride-source-workflow.md). Публикация fork/релиза не выполнялась. Следующий игровой шаг — адаптация существующей P1 к Stride/C#.

## Bugs found

В ходе ревью исправлены относительные пути PowerShell, wildcard-обработка путей сборки/журналов и ложноположительные guard-тесты. Незакрытых ошибок этого среза: none.
