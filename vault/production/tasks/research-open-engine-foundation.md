---
type: task
area: Game
status: Done
task_type: Research
sprint: Sprint 19
review: Approved
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

Сравнение подготовлено; пользователь выбрал Stride. Решение закреплено в [[ADR-018 Rat expedition uses Stride]], а варианты MCP и сопутствующих инструментов — в [технической записке](../../../docs/stride-mcp-integration-research.md). Документационный срез `60c253d` получил независимое read-only ревью `stride_research_review`: **Approved**, блокирующих замечаний нет. Установка MCP и перенос игры не выполнялись.

История сравнения, первичные источники и проверки сохранены в [отчёте](../../game/research/technical-open-game-engine-foundation-for-rat-expe-2026-09-08/research.md), commit `667b194`; прежняя рекомендация Godot явно помечена исторической после выбора Stride. Следующий шаг — адаптация P1 к Stride и проверка интеграции. При детализации отличать development-пакет с runtime-мостом от распространяемого Release-пакета без моста.

Validation 2026-09-08: `scripts/verify.ps1` завершился с exit 0; 16 Python tests, 720 CTest passed и один symlink privilege skip из 721. Полный Release-редактор: `C:/5_gamedev/rat-engine/build/dev-release/apps/editor/rat-editor.exe`. Vault checker и BMad projection check прошли. Это проверка текущей рабочей базы rat-engine; GUI, Stride runtime и MCP ею не проверены.

Уточнение пользователя 2026-09-08: выбран **Stride**; исследование продолжается в части MCP, управления Game Studio, диагностики игрового процесса, сборки и тестирования. Сравнение пяти движков сохраняется как история. Следующий результат — зафиксированное решение и техническая записка о вариантах интеграции; установка MCP и перенос P1 не входят в это исследование.

## Bugs found

none в изменениях проекта. В стороннем AkerMCP отмечены статические ограничения и риск ожидания процесса сборки; они записаны в исследовании и не выдаются за воспроизведённые дефекты rat-engine.
