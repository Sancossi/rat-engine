---
type: task
area: Engine
status: In review
task_type: Chore
sprint:
due:
tags: [task, expedition, cleanup, stride]
---

# Удаление прежней C++-итерации

Intent: По утверждённому пользователем плану убрать прежний движок, редактор, сборочную инфраструктуру и локальные C++-артефакты после перехода на Stride/C#. Отдельная работа по запросу пользователя; продолжение очереди Sprint 19 не входит в объём.

Specification: [План очистки](../../../docs/cpp-retirement-plan.md).

Acceptance: C++-реализация удалена; исторические документы доступны в архиве с рабочими ссылками; ресурсы и сборки Stride сохранены; vault/Python и новый Release-пакет проверены; независимое review Approved; Release Game Studio пересобран.

Origin: [[ADR-018 Rat expedition uses Stride]].

## Resolution

В работе. Последний снимок до очистки: `60fdc55f19889304fc29957fd2124624077aefcd`.

## Bugs found

Проверка ещё не завершена.
