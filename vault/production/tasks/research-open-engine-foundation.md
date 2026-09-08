---
type: task
area: Game
status: In progress
task_type: Research
sprint: Sprint 19
review: Pending
baseline_commit: 0f5bf2f
due:
tags: [task, expedition, research, engine]
---

# Сравнить пять открытых движков для Rat Expedition

Intent: Проверить, стоит ли заменить rat-engine до продолжения P1, сравнив Godot, Stride, Wicked Engine, O3DE и id Tech 4 на одинаковых требованиях проекта.

Specification: [[GDD]]; [[ADR-017 Rat expedition uses rat-engine]]; [предыдущее сравнение](../../../docs/rat-expedition-engine-comparison.md); [[expedition-prototype-traversal]].

## Acceptance

- Для пяти кандидатов проверены актуальная лицензия, доступность исходников, Windows x64 поставка, 2D-персонажи в 3D, authoring, тестирование, миграция и здоровье экосистемы.
- Решение показано взвешенной матрицей: скорость разработки 25%, соответствие P1 20%, лицензия и контроль 15%, миграция 15%, тестирование и Windows-поставка 15%, зрелость и долгосрочный риск 10%.
- Указаны победитель, второй кандидат, сильнейший аргумент против победителя и минимальный обратимый spike; rat-engine приведён как текущая контрольная база.
- Все определяющие выводы имеют свежие первичные источники и независимую проверку; пробелы и устаревание отмечены явно.
- Результат не меняет ADR-017 автоматически: смена движка требует отдельного решения пользователя.

## Границы

Не переносить P1 и не добавлять SDK движков. Не копировать проприетарный код или игровые данные. Исследование сравнивает возможность и цену перехода.

Origin: [[expedition-prototype-traversal]]; [[ADR-017 Rat expedition uses rat-engine]].
Follow-up: [[expedition-prototype-traversal]].

## Resolution

В работе.

## Bugs found

Пока не проверено.
