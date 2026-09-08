---
type: sprint
status: In Progress
dates: 2026-09-08
goal: Resolve Stride integration and revise the expedition traversal prototype plan
current: true
tags: [sprint, expedition, prototype]
---

# Sprint 19 — Expedition traversal prototype

## Порядок

1. [[research-open-engine-foundation]]: сравнить пять открытых движков и получить решение пользователя по основе проекта.
2. [[feat-stride-source-foundation]]: подготовить закреплённую исходную копию официального Stride и проверить сборку Game Studio.
3. [[expedition-prototype-traversal]]: после исходной основы адаптировать архитектуру и критерии P1 к Stride; исторические C++ срезы не продолжать.

Реализация по [спецификации](../../../docs/rat-expedition-traversal-spec.md) и [архитектуре](../../../docs/rat-expedition-architecture.md), включая оба визуальных ориентира и [[rat-expedition-layered-maps]]. Другие игровые карточки и закрытые задачи старого движка не входят в спринт.

## Resolution

В работе. Пользователь 2026-09-08 выбрал Stride; исследование MCP и подготовка исходной основы завершены с независимым ревью. Game Studio собран из закреплённого upstream и прошёл startup smoke. P1 остаётся заблокирован до адаптации архитектуры и плана к Stride/C#; прежние C++ срезы не продолжаются.

## Bugs found

Пока не проверено.
