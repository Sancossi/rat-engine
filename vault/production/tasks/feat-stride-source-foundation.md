---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 19
review: Pending
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

В работе. Авторизован локальный переход и сборка из исходников. Публикация GitHub fork или релиза сейчас не запрошена.

## Bugs found

Пока не проверено.
