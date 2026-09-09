---
type: task
area: Game
status: Done
task_type: Research
sprint: Sprint 19
due:
review: Approved
tags: [task, expedition, stride, design]
---

# План работы с картами и ассетами в Game Studio

Intent: По запросу пользователя 2026-09-09 запланировать открытие и визуальное редактирование карт и остальных игровых ассетов в пересобранном Stride Game Studio, с сохранением изменений в запускаемой игре.

Specification: [[ADR-018 Rat expedition uses Stride]]; [[expedition-prototype-traversal]]; [архитектура](../../../docs/rat-expedition-architecture.md); [план до релиза](../roadmap/rat-expedition-release-roadmap.md). Сейчас сцена создаётся из JSON программно, редактор не имеет её scene asset. Уточнение пользователя «Так же и все остальные ассеты» включено в этот же запрос.

## Acceptance

- Есть последовательный план проекта Game Studio, миграции карты и библиотек ресурсов, связи с Core и сборки самостоятельной игры.
- Для карт, моделей, материалов, текстур, спрайтов, анимаций, звука, UI/шрифтов, prefab и используемых игровых metadata определены источник, редактирование и проверка сохранения/повторного открытия.
- Один авторский источник определяет вид и коллизии; правила Core/FSM сохраняются. Импорт ресурсов и создание исходного арта во внешних инструментах различаются.
- Будущая реализация имеет отдельную карточку Not started, зависимости, место в дорожной карте и GUI-критерии приёмки. Запрос планирования не запускает runtime-миграцию.
- Независимое документальное ревью, vault checks и требуемая Release-сборка редактора записаны с ограничениями доказательств.

Origin: [[expedition-prototype-traversal]].
Follow-up: [[feat-stride-game-studio-authoring]]; [[design-expedition-character-sprites]].

Related: [План авторинга A1](../../../docs/stride-editor-asset-workflow-plan.md).

## Resolution

2026-09-09: план `f2a31ee` принят после независимого read-only ревью: Approved, документальных блокеров нет. Четыре среза описывают квалификацию native проекта, перенос карт с единым источником геометрии/коллизий, библиотеку ресурсов и проверку GUI/самостоятельной игры. Покрыты импорт и повторный импорт, сохранение/повторное открытие, undo/redo, ссылки/идентификаторы, ошибки и происхождение ресурсов. Создана [[feat-stride-game-studio-authoring]] со статусом Not started без спринта; плановый порядок — полный P1 → A1 → P2, до массового контента E3. Связанные архитектура, roadmap и зависимости согласованы. Реализация, карты и ассеты не менялись.

Vault checks и `git diff --check` passed; BMad projection синхронизирована из vault. Полная Release-сборка Game Studio: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build.ps1`, exit 0, 43.07 с, пять upstream NU5100 warnings / ноль ошибок. Evidence: `C:/5_gamedev/stride/logs/rat-foundation/20260909-044225-060/result.json` и `build.log`; editor: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Закреплённый source HEAD `e2c786a45f69917bf233793f6a097b150e2fe264` остался чистым. Эта сборка подтверждает основу редактора; редактирование ресурсов, GUI-сценарии и новый ZIP этим документационным этапом не проверялись.

## Bugs found

none — открытых замечаний документального ревью нет. Реальная совместимость asset pipeline и GUI подлежит квалификации в A1.1; готовность будущей функции не заявляется.
