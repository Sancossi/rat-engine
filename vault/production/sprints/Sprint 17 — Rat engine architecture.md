---
type: sprint
status: Done
dates: 2026-09-08
goal: Record the user choice of rat-engine and define the expedition architecture and first prototype slice
current: false
tags: [sprint, expedition, architecture]
---

# Sprint 17 — Rat engine architecture

Пользователь выбрал собственный движок после [[Sprint 16 — Rat expedition preproduction]].
Продолжаем существующий rat-engine; фиксируем технические границы новой игры.

## Порядок

1. [[expedition-engine-decision]] — принятое решение, архитектура и уточнение P1.

## Границы

Документы и план первой сцены. Реализация [[expedition-prototype-traversal]] и
остальных игровых карточек остаётся следующим этапом; старые спринты закрыты.

## Resolution

2026-09-08: E1 закрыт с Approved; выбран существующий rat-engine, записаны ADR-017, архитектура и спецификация первой сцены. После закрытия verify.ps1: exit 0, полный Release editor, 714/714 CTest и 16/16 Python. [Отчёт приёмки](../../../docs/audits/2026-09-08-rat-expedition-engine-architecture.md). P1 остаётся Not started вне спринта; current false и доска без активного спринта.

## Bugs found

Исправлена одна неточность спецификации ортографических перекрытий; повторное независимое ревью Approved. Незакрытых находок нет.
