---
type: task
area: Game
status: In progress
task_type: Feature
sprint: Sprint 19
review: Pending
due:
tags: [task, expedition, art, blender]
---

# Канальный город 1 — основа и эталон

Intent: Реализовать первый этап утверждённого полного каталога Blender → Stride: полный реестр, общая основа, шесть подробных модулей, анимация двери и собранный фрагмент.

Specification: [Каталог](../../../docs/canal-city-catalog-spec.md).

Dependencies: Blender source не зависит от A1; Stride приёмка требует завершения A1.3/A1.4 в [[feat-stride-game-studio-authoring]]. Это не разрешение продолжать чужую незавершённую MCP работу.

Acceptance:
- Given три листа, when реестр готов, then все уникальные варианты имеют ID, размеры и источник, повторные ракурсы дедуплицированы.
- Given запуск генератора, when выполнен export, then шесть моделей, текстуры, редактируемый .blend, дверные клипы и эталонные рендеры существуют и независимо проверены.
- Given готовый A1, when импорт/повторный импорт/Save/reopen/run выполнены, then mesh/material/animation корректны в Stride и standalone ZIP.
- Given полный результат, when independent review одобрено, then evidence и Release редактор записаны до закрытия и публикации.

Origin: [[rat-expedition-art-audio]].
Follow-up: [[feat-canal-city-02-passages]]; [[feat-canal-city-03-architecture]]; [[feat-canal-city-04-mechanisms]]; [[feat-canal-city-05-decoration]]; [[feat-canal-city-06-library]].

## Resolution

2026-09-09: пользователь запросил исполнение утверждённого плана. Выделена чистая feature worktree от main, исходная копия с текущими MCP изменениями сохранена. Начато изготовление первого набора; A1 и весь каталог не объявляются завершёнными.

## Bugs found

Проверка ещё не завершена.
