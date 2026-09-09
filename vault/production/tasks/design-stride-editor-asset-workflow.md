---
type: task
area: Game
status: In review
task_type: Research
sprint: Sprint 19
due:
review: In review
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
Follow-up: [[feat-stride-game-studio-authoring]].

Related: [План авторинга A1](../../../docs/stride-editor-asset-workflow-plan.md).

## Resolution

Планирование начато; runtime и ассеты этим заданием не меняются.

## Bugs found

Пока не проверено.
