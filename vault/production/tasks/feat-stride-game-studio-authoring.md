---
type: task
area: Game
status: Not started
task_type: Feature
sprint:
due:
tags: [task, expedition, stride, authoring]
---

# Карты и игровые ресурсы в Stride Game Studio

Intent: Открывать, размещать и настраивать карты и остальные категории игровых ресурсов в пересобранном Game Studio, сохраняя один авторский источник для запуска из редактора и самостоятельной игры.

Specification: [План A1 и приёмка](../../../docs/stride-editor-asset-workflow-plan.md); [архитектура](../../../docs/rat-expedition-architecture.md); [[ADR-018 Rat expedition uses Stride]].

Dependencies: [[expedition-prototype-traversal]] целиком, включая P1.3; [[feat-stride-source-foundation]]; [[design-stride-editor-asset-workflow]]. Предлагаемое место A1 — после P1 перед P2, до массового контента E3. Карточка вне спринта; планирование не запускает реализацию.

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

Реализация не начиналась. При выполнении фиксировать результаты срезов A1.1–A1.4 из плана, не создавать параллельный источник статусов.

## Bugs found

Не проверено: это план будущей реализации, runtime и ресурсы не изменены.
