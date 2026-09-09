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
Follow-up: [[expedition-prototype-interactions]]; [[feat-expedition-character-sprites]]; [[expedition-vertical-slice]]; [[feat-stride-editor-mcp]].

## Resolution

Обновление после приёмки MCP 2026-09-09: [[feat-stride-editor-mcp]] — Done / Approved, зависимость снята. A1.2 возобновляется в сохранённой основной рабочей копии. Parent через API прочитал 30 entities Courtyard, открыл сцену и получил реальный viewport PNG; файлы A1.2 не менялись. Wall edit/save/reopen/collision, native verifier 33, упаковка и независимое review A1.2 остаются обязательными; A1.3 и A1.4 ещё не приняты.

Обновление 2026-09-09: пользователь явно поручил [[feat-stride-editor-mcp]] для управления через API. A1.2 ожидает этот мост; его незакоммиченная реализация сохранена в основной рабочей копии. Пройдены Core 63, adapter 14, native build и layered GPU route, parent сравнил обе карты с прежними данными (146 численных полей, max delta 2e-7). Не завершены GUI/API wall edit/save/reopen/collision, адаптация verifier 33, итоговая упаковка и независимое ревью A1.2. Это не отмена A1 и не закрытие P1.

Начато 2026-09-09 по явному разрешению пользователя. Порядок: A1.1 native project/compiler/GUI квалификация → A1.2 карты/Core → A1.3 библиотека ресурсов → A1.4 единый запуск/ZIP/общая приёмка. Один исполнитель, независимое read-only ревью каждого среза. Оставшийся ручной P1 перенесён на позже; его статус не закрывается автоматически.

A1.1 передан на независимое ревью: `fcd1fd7`. Native scene/custom metadata прошли реальный GUI edit/undo/redo/save/close/reopen/F5; wrapper загрузил те же id, значение и изображение. [Контракт среза](../../../docs/stride-native-authoring-qualification-spec.md), [доказательства](../../../docs/audits/2026-09-09-stride-native-authoring-qualification.md). Это проверка основы; карты/Core и библиотека ресурсов ещё не перенесены. A1 целиком не закрыт.

A1.1 принят 2026-09-09: независимое read-only review `ffeab7a..fcd1fd7` — Approved. Parent повторил wrapper из committed HEAD `a302c75`, authoringWorkingTreeDirty=false, exit 0; результат `build/stride-authoring/20260909-094802-255/result.json`, JSON/PNG совпадают с F5. Core: 63 passed; Python: 22 passed. Полный Release Game Studio пересобран, exit 0, 5 upstream NU5100 warnings / 0 errors: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`; evidence `C:/5_gamedev/stride/logs/rat-foundation/20260909-094805-068/result.json`. Исполняемые P1 сценарии повторно не запускались: основной runtime и данные не изменены, их последняя приёмка записана в [[bug-expedition-background-smoke-speed]].

Начат A1.2: перенос обеих игровых карт (courtyard/sluice), metadata и общей валидации/Core boundary. A1.1 Approved относится только к законченному срезу; review Pending относится к продолжающейся реализации A1.2. Ручной P1 по-прежнему отложен по решению пользователя.

## Bugs found

A1.1: none; независимое ревью не выявило блокирующих дефектов. A1.2 ещё выполняется.
