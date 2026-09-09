---
type: sprint
status: In Progress
dates: 2026-09-08
goal: Implement the Stride expedition traversal prototype in verified slices
current: true
tags: [sprint, expedition, prototype]
---

# Sprint 19 — Expedition traversal prototype

## Порядок

1. [[research-open-engine-foundation]]: сравнить пять открытых движков и получить решение пользователя по основе проекта.
2. [[feat-stride-source-foundation]]: подготовить закреплённую исходную копию официального Stride и проверить сборку Game Studio.
3. [[expedition-prototype-traversal]]: после исходной основы адаптировать архитектуру и критерии P1 к Stride; исторические C++ срезы не продолжать.
4. [[publish-editor-game-vault-snapshot]]: по отдельному запросу пользователя опубликовать общий снимок после коррекции камеры P1.1a, до продолжения P1.2.
5. [[design-stride-editor-asset-workflow]]: по запросу пользователя запланировать визуальную работу с картами и всеми категориями игровых ассетов в Game Studio; только документы, без запуска миграции.

Реализация по [спецификации](../../../docs/rat-expedition-traversal-spec.md) и [архитектуре](../../../docs/rat-expedition-architecture.md), включая оба визуальных ориентира и [[rat-expedition-layered-maps]]. Другие игровые карточки и закрытые задачи старого движка не входят в спринт.

## Resolution

В работе. Исследование MCP и исходная основа Stride завершены. Архитектура и P1 адаптированы к Stride/C#. P1.1 принят 2026-09-09: самостоятельная Windows-игра, серый двор/PNG, WASD/стены, ZIP, два GPU-разрешения и проверки отказов; независимое ревью Approved. P1.1a принят 2026-09-09: камера приближена и следует за героем, колесо регулирует зум; финальный ZIP прошёл 10 executable сценариев, review Approved. По запросу пользователя общий снимок редактора, игры и vault опубликован в ветке publish/stride-p11-editor-game-vault; карточка публикации Done / review Approved. P1.2 принят 2026-09-09: явная FSM, полный объём тела, приседание/лаз и лестница/выходы; 28 Core и 17 executable сценариев passed, независимое ревью Approved. Следующий срез P1.3 — две сцены/переходы/recovery/пауза, мост/рампа, спутники и локальные перекрытия. Полная карточка P1 и спринт ещё не закрыты; прежняя C++ реализация сохраняется отдельно.

План [[design-stride-editor-asset-workflow]] принят 2026-09-09: review Approved. Будущая [[feat-stride-game-studio-authoring]] остаётся Not started вне спринта; предусмотрены карты и остальные категории ассетов, единый источник данных для редактора/игры и GUI/ZIP приёмка. Плановый порядок — полный P1 → авторинг A1 → P2. Это завершение планирования, не начало переноса ресурсов.

## Bugs found

Исправленные дефекты P1.1/P1.1a/P1.2 записаны в [[expedition-prototype-traversal]] и связанных audit. Открытых дефектов принятых срезов: none. P1.3 ещё не проверен.
