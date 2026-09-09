---
type: task
area: Game
status: Not started
task_type: Feature
sprint: Sprint 19
review: Pending
due:
tags: [task, expedition, art, blender]
---

# Канальный город 6 — полная библиотека и приёмка

Intent: Полный native каталог, три обзорные сцены и игровой фрагмент, reimport/reopen/клипы/occlusion/ZIP, документация размещения.

Specification: [Утверждённый каталог](../../../docs/canal-city-catalog-spec.md).

Dependencies: [[feat-canal-city-05-decoration]]; Stride приёмка после A1.3/A1.4 в [[feat-stride-game-studio-authoring]].

Acceptance:
- Given реестр, when этап изготовлен, then все его уникальные варианты имеют редактируемый source, экспорт, материалы и необходимые клипы, отсутствующие позиции явно выявляются проверкой.
- Given Blender/Stride, when выполнены независимые load/import и визуальная проверка, then соблюдены размеры, силуэты и анимации, evidence различает источник и игровой результат.
- Given independent review Approved, when выполнены применимые проверки и полный Release editor, then записаны результаты до закрытия и публикации.

Origin: [[feat-canal-city-01-foundation]]; [[feat-canal-city-05-decoration]].

## Resolution

Запланировано пользователем 2026-09-09; pickup после зависимости, без автоматического закрытия предыдущих этапов.

## Bugs found

Проверка ещё не начата.

