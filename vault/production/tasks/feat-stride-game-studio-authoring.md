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

A1.2 опубликован в `rat-engine/main`: `ff24f314e3d5b3f08f955757b6516c301be65ff4`, remote SHA проверен; `Sancossi/stride/main` остаётся на принятом `88301e861149c48c8b408aac3030b190332d7f97`. Посторонние два vault EOL changes сохранены. Начат A1.3 по утверждённой таблице ресурсов: один представитель обязательной категории, native авторский источник, runtime/ZIP использование, API edit/save/reopen и import/reference qualification. Сначала исполнитель фиксирует конкретные assets/API и границы проверки в инженерной спецификации, затем реализует этот срез; A1.4 и ручной P1 остаются отдельными. Review Pending относится к A1.3, принятие A1.2 сохраняется.

Работа MCP по умолчанию опирается на структурированные данные без viewport_capture; изображения нужны только отдельным визуальным критериям. Существующие 12 команд не объявляются API всего asset catalog/import: недостающая часть должна быть явно квалифицирована в A1.3.

Финальное review документации — Approved `ac4f88e`: README явно разделяет production ZIP и QA build/verifier. A1.2 полностью принят как срез; оба runtime P2 и ошибка команды устранены. Публикуется проверенная история до этого acceptance commit; A1.3/A1.4 остаются незавершёнными.

Финальная проверка документации выявила неверный quick start: обычный production ZIP передаётся полному verifier, которому теперь нужны QA assets. До публикации исправить пример в README игры и повторить read-only проверку. Одобрение runtime `49a9923` и результаты всех финальных проверок сохраняются; мерж пока не выполнен.

A1.2 принят 2026-09-09: независимое повторное review `49a9923` — Approved, оба P2 исправлены. Native Courtyard/Sluice открываются и сохраняются в Game Studio; реальный MCP wall edit/Undo/Redo/reopen меняет compiled вид и Core collision из одного источника. Финальные QA и обычный ZIP собраны из `b403f84` (исправленный runtime, gameWorkingTreeDirty=false), Core 63 / Authoring 15 passed. Все 36 executable сценариев прошли: 21 успешный запуск и 15 ожидаемых отказов, `C:/5_gamedev/rat-expedition-validation/20260909-125458-633/verification.json`. Обычный ZIP `build/stride-game/20260909-125515-649/rat-expedition-0.1.0-win-x64.zip` отдельно распакован и запущен с другим cwd; обе native карты совпадают с QA, legacy map/project JSON в пакете нет. Parent повторил численную parity исправленного пакета: 2 карты / 146 полей / max delta 2e-7 / errors[].

Полный Release Game Studio пересобран: `C:/5_gamedev/stride/logs/rat-foundation/20260909-125243-034/result.json`, exit 0, 5 прежних NU5100 warnings / 0 errors. Редактор: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Stride main/pin остаётся `88301e8`, исходники движка в A1.2 не менялись. [Итоговые доказательства и SHA256](../../../docs/audits/2026-09-09-stride-native-map-authoring.md). Проверка выполнена на developer PC; новый ручной F5/playtest и чистая машина не заявлены. Публикация принятого среза в main выполняется по постоянному поручению пользователя. Review Approved относится к A1.2; вся A1 остаётся In progress, далее A1.3 библиотека ресурсов и A1.4 общая приёмка.

A1.2 исправления переданы на повторное review: `49a9923`. Parent translation и границы считаются с wider intermediates, float округляется на конечной границе; Core exact contact не ослаблен. Новый тест воспроизвёл отказ до исправления, затем authoring 15 passed, включая отрицательный Y и отказ реально висящей стены. Preview выполняется до Transform/ModelRender processors. В свежем редакторе прошли 85 MCP вызовов Size/Role/Undo/Redo/Save/reopen и 17 проверок готового непустого кадра, diagnostics 0/0; PNG не сохранялись и не использовались для управления. Исходные параметры стены восстановлены. Ожидаются повторное review и итоговые проверки исправленной версии.

A1.2 review `857e3a1` — Needs fixes: допустимый общий parent Y=.1 даёт разные float wall bottom/floor top после centre/size-конверсии; GeometryPreviewProcessor.Draw может освобождать buffers после их включения ModelRenderProcessor в текущий кадр (оба Order=0). Исправить в этом срезе и повторить независимое review до мержа. Committed QA package `20260909-122553-348` прошёл все 36 executable сценариев (`C:/5_gamedev/rat-expedition-validation/20260909-122643-970/verification.json`); это не отменяет замечаний review. Полная A1 приёмка остаётся открытой.

A1.2 передан на независимое review: `857e3a1`, native Courtyard/Sluice, GUID/Core adapter и compiled QA fixtures. Реальный MCP wall edit/Undo/Redo/Save/reopen выполнен без имитации ввода: 37 вызовов для сдвига и 32 для восстановления. Compiled Core sweeps меняют old/new collision и возвращаются после восстановления; parent сравнил 146 численных полей обеих восстановленных карт с историческим baseline, max delta 2e-7, errors[]. [Доказательства](../../../docs/audits/2026-09-09-stride-native-map-authoring.md). Core 63 / authoring 14 и узкие native gates прошли; полный committed-head verifier ещё выполняется. Статус In review относится к A1.2; вся карточка A1 и A1.3/A1.4 не закрыты.

Возобновлено 2026-09-09 по «Давай продолжим выполнение задач». Следующий срез — завершение A1.2 по [спецификации native карт](../../../docs/stride-native-map-authoring-spec.md), используя принятый MCP. Новое постоянное поручение: после приёмки и закрытия каждого этапа/спринта мержить проверенный результат в main; при изменении исходников Stride также обновлять main собственного fork и точный pin. Правило записано в AGENTS/source workflow. Сохранённые A1.2 изменения продолжаются в feat/stride-native-authoring, посторонние изменения Task board.base и task template сохраняются отдельно.

Обновление после приёмки MCP 2026-09-09: [[feat-stride-editor-mcp]] — Done / Approved, зависимость снята. A1.2 возобновляется в сохранённой основной рабочей копии. Parent через API прочитал 30 entities Courtyard, открыл сцену и получил реальный viewport PNG; файлы A1.2 не менялись. Wall edit/save/reopen/collision, native verifier 33, упаковка и независимое review A1.2 остаются обязательными; A1.3 и A1.4 ещё не приняты.

Обновление 2026-09-09: пользователь явно поручил [[feat-stride-editor-mcp]] для управления через API. A1.2 ожидает этот мост; его незакоммиченная реализация сохранена в основной рабочей копии. Пройдены Core 63, adapter 14, native build и layered GPU route, parent сравнил обе карты с прежними данными (146 численных полей, max delta 2e-7). Не завершены GUI/API wall edit/save/reopen/collision, адаптация verifier 33, итоговая упаковка и независимое ревью A1.2. Это не отмена A1 и не закрытие P1.

Начато 2026-09-09 по явному разрешению пользователя. Порядок: A1.1 native project/compiler/GUI квалификация → A1.2 карты/Core → A1.3 библиотека ресурсов → A1.4 единый запуск/ZIP/общая приёмка. Один исполнитель, независимое read-only ревью каждого среза. Оставшийся ручной P1 перенесён на позже; его статус не закрывается автоматически.

A1.1 передан на независимое ревью: `fcd1fd7`. Native scene/custom metadata прошли реальный GUI edit/undo/redo/save/close/reopen/F5; wrapper загрузил те же id, значение и изображение. [Контракт среза](../../../docs/stride-native-authoring-qualification-spec.md), [доказательства](../../../docs/audits/2026-09-09-stride-native-authoring-qualification.md). Это проверка основы; карты/Core и библиотека ресурсов ещё не перенесены. A1 целиком не закрыт.

A1.1 принят 2026-09-09: независимое read-only review `ffeab7a..fcd1fd7` — Approved. Parent повторил wrapper из committed HEAD `a302c75`, authoringWorkingTreeDirty=false, exit 0; результат `build/stride-authoring/20260909-094802-255/result.json`, JSON/PNG совпадают с F5. Core: 63 passed; Python: 22 passed. Полный Release Game Studio пересобран, exit 0, 5 upstream NU5100 warnings / 0 errors: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`; evidence `C:/5_gamedev/stride/logs/rat-foundation/20260909-094805-068/result.json`. Исполняемые P1 сценарии повторно не запускались: основной runtime и данные не изменены, их последняя приёмка записана в [[bug-expedition-background-smoke-speed]].

Начат A1.2: перенос обеих игровых карт (courtyard/sluice), metadata и общей валидации/Core boundary. A1.1 Approved относится только к законченному срезу; review Pending относится к продолжающейся реализации A1.2. Ручной P1 по-прежнему отложен по решению пользователя.

## Bugs found

A1.2: два P2 в конверсии Y и времени жизни editor preview buffers выявлены независимым review и исправлены в `49a9923`; повторное review Approved. Остаточных блокирующих дефектов не найдено.

A1.1: none; независимое ревью не выявило блокирующих дефектов. A1.2 ещё выполняется.
