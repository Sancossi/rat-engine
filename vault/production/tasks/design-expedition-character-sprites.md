---
type: task
area: Game
status: Done
task_type: Research
sprint: Sprint 19
due:
review: Approved
tags: [task, expedition, art, design]
---

# План персонажей и ходьбы по пользовательскому листу

Intent: По запросу пользователя 2026-09-09 запланировать добавление трёх различимых персонажей и направленной анимации ходьбы по приложенному изображению.

Specification: [План трёх обликов и ходьбы](../../../docs/stride-character-sprite-plan.md); [[rat-expedition-character-reference]]; [[GDD]]; [[rat-expedition-art-audio]]; [[feat-stride-game-studio-authoring]]; [план A1](../../../docs/stride-editor-asset-workflow-plan.md). Источник — вложение текущего разговора; локальный файл и готовый импорт не заявлены.

## Acceptance

- Описаны наблюдения по листу, граница между референсом и готовым atlas, неопределённости размеров/кадров и возможного прямого импорта.
- План охватывает подготовку кадров/прозрачности/pivot, отдельные образы трёх героев, ходьбу/idle и явные пробелы crouch/climb/fall.
- Анимация следует существующей FSM и фактическому 3D движению, корректно останавливается в паузе и воспроизводится спутниками по истории.
- Импорт/правка метаданных/preview/save/reopen в Game Studio и самостоятельный ZIP связаны с A1, без второго ручного runtime-источника.
- Создана будущая карточка реализации Not started вне спринта; связь с A1/E3 и бюджетом A7 согласована без запуска реализации.
- Документальное независимое ревью, vault check и требуемая Release-пересборка редактора записаны.

Origin: [[design-stride-editor-asset-workflow]]; [[rat-expedition-art-audio]].
Follow-up: [[feat-expedition-character-sprites]]; [[rat-expedition-character-reference]].

## Resolution

Принято 2026-09-09. Коммит плана `fa72bf05dea8e5987d6e5be6e524eeb0cdd057c3`: три отдельных облика, пилот одного героя, подготовка alpha/pivot/кадров, анимация по движению/FSM и истории спутников, будущая GUI/ZIP приёмка через A1. Сохранены наблюдения по вложению и открытый выбор reference/direct import. [[feat-expedition-character-sprites]] остаётся Not started вне спринта, после A1 и до тиражирования персонажей E3; новая зависимость A1/P2 не введена. Игра и изображения не изменялись.

Независимое read-only ревью `stride_blind_review` коммита `fa72bf0` на HEAD `8c33dc8`: Approved, блокирующих замечаний нет. Область — девять документационных файлов; реальные спрайты и GUI этим ревью не проверялись. `python scripts/check_vault.py` и `git diff --check` прошли; проекция sprint синхронизирована и проверена через `scripts/bmad_vault.py` при закрытии.

Полная Release-пересборка: `powershell -ExecutionPolicy Bypass -File scripts/stride/build.ps1`, exit 0, 5 upstream NU5100 предупреждений / 0 ошибок. Evidence: `C:/5_gamedev/stride/logs/rat-foundation/20260909-064908-697/result.json`; upstream `e2c786a45f69917bf233793f6a097b150e2fe264`, SDK 10.0.300. Редактор 4.4.0-dev: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Это проверка сборки, не GUI-импорта или новых игровых ассетов.

## Bugs found

none — блокирующих дефектов документов не найдено; игровой runtime в этом плановом срезе не проверялся.
