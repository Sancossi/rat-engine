---
type: task
area: Game
status: In progress
task_type: Feature
sprint: Sprint 19
review: Pending
due:
tags: [task, expedition, stride, authoring]
---

# Карты и игровые ресурсы в Stride Game Studio

Intent: Открывать, размещать и настраивать карты и остальные категории игровых ресурсов в пересобранном Game Studio, сохраняя один авторский источник для запуска из редактора и самостоятельной игры.

Specification: [План A1 и приёмка](../../../docs/stride-editor-asset-workflow-plan.md); [архитектура](../../../docs/rat-expedition-architecture.md); [[ADR-018 Rat expedition uses Stride]].

Dependencies: реализация и автоматическая приёмка [[expedition-prototype-traversal]], включая P1.3; [[feat-stride-source-foundation]]; [[design-stride-editor-asset-workflow]]. Пользователь 2026-09-09 явно разрешил начать A1, перенеся оставшуюся ручную приёмку P1 на позже. Это исключение из прежнего порядка «полностью закрытый P1 → A1», не утверждение о выполнении ручного теста. A1 включён в Sprint 19 перед P2, до массового контента E3.

Acceptance:

- Квалифицирован native проект/пакет/compiler на закреплённом Stride; записаны реальные source anchors и GUI запуск без подмены stable пакетами.
- Двор и остальные фактические сцены P1 редактируются как authored assets. Stable ids, solid geometry/Core volumes, spawn/лестницы/переходы используют один источник без ручного JSON-дубля; неподдерживаемые transforms отвергаются.
- Карты/prefab, модели, текстуры/материалы, sprite sheets/анимации, аудио, UI/шрифты, gameplay metadata, исходники/лицензии проверены представительными ресурсами; VFX учтён при использовании либо явно N/A. Внешний source editing отделён от native import/properties.
- Реальный GUI цикл open/edit/undo/redo/save/close/reopen/run подтверждён. Перемещение стены независимо меняет вид и столкновение; невалидные spawn/ladder, duplicate ids, rename/delete references и повреждённый импорт имеют явное поведение/ошибки.
- Сборка и обычный запуск из редактора используют те же authored assets. Извлечённый committed-head self-contained ZIP работает с другим cwd без editor/SDK/source/network; проверены ресурсы, мир и кадры 720p/1080p.
- Сохранены FSM, полный P1, камера/feet Y/depth; адаптированы регрессии и инструкции. Независимое ревью и Release Game Studio сопровождаются точными доказательствами и границами GUI/изолированной проверки.

Origin: [[design-stride-editor-asset-workflow]]; [[expedition-prototype-traversal]].
Follow-up: [[expedition-prototype-interactions]]; [[feat-expedition-character-sprites]]; [[expedition-vertical-slice]].

## Resolution

Начато 2026-09-09 по явному разрешению пользователя. Порядок: A1.1 native project/compiler/GUI квалификация → A1.2 карты/Core → A1.3 библиотека ресурсов → A1.4 единый запуск/ZIP/общая приёмка. Один исполнитель, независимое read-only ревью каждого среза. Оставшийся ручной P1 перенесён на позже; его статус не закрывается автоматически.

## Bugs found

Не проверено: это план будущей реализации, runtime и ресурсы не изменены.
