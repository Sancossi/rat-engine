---
title: Очистка после перехода на Stride
type: chore
created: 2026-09-09
baseline_commit: 60fdc55f19889304fc29957fd2124624077aefcd
context: [AGENTS.md, docs/bmad/project-context.md]
---

# Очистка после перехода на Stride

Единственный источник статуса: [карточка](../vault/production/tasks/chore-retire-cpp-iteration.md).

<frozen-after-approval reason="Утверждено пользователем в плане и запросе реализации">

Удалить прежний C++-движок, редактор и инфраструктуру сборки. Сохранить работающую игру Stride, её ресурсы, дизайн и историю проекта. Исходники C++ остаются доступны через Git без переписывания истории. Игровые механики, форматы и версии зависимостей не меняются; продолжение спринта не входит в объём.

Удалить apps, src, tests, benchmarks, корневые CMake-файлы, cmake, старые assets и data. Перенести содержательную документацию из удаляемых каталогов в docs/archive/cpp. Удалить scripts/verify.ps1, check_msvc_dependencies.ps1, run_benchmark.py, run_clang_tidy.py, run_gui_acceptance.py, gui_scenarios.json и scripts/tests/test_clang_tidy.py. Сохранить Stride/BMad/vault scripts и остальные Python tests. В CI оставить repository checks, убрать C++ jobs; новая полная Stride CI не входит в объём.

Перенести C++-спецификации, схемы, планы, benchmark и audit evidence в docs/archive/cpp с сохранением внутренней структуры. Общие материалы vault/BMad, исследования выбора движка и Stride остаются действующими. Исправить Markdown links и wikilinks; исторические исходные пути относятся к baseline_commit. Сохранить карточки, решения и закрытые спринты; уточнение отказа от сохранения C++ записать датированным дополнением к ADR-018.

Обновить README, AGENTS, Cursor rules, BMad context, действующие спецификации и rat-build-verify/rat-runtime-repro под Stride. Сохранить toolchain и native-компоненты, необходимые upstream Stride. Уточнить credits ресурсов без потери авторства и лицензий.

Удалить только подтверждённые локальные C++-сборки, бинарные пакеты, clangd caches и aqtinstall.log. Перед удалением сохранить отдельные отчёты и снимки в build/cpp-history. Сохранить build/stride-game, игровые NuGet caches, общие инструменты и соседний Stride checkout. Перед рекурсивными операциями проверить абсолютные пути; неизвестные локальные файлы сохранять. Не переписывать Git history и не публиковать remote изменения.

</frozen-after-approval>

## Выполнение

Один исполнитель в общей ветке вносит и коммитит изменения, не меняя статусы карточек. Независимый read-only reviewer проверяет diff и результаты. Ведущий агент отвечает за статусы и закрытие после исправления замечаний.

## Проверка и приёмка

- После удаления старых каталогов `python scripts/check_vault.py` и `python -m unittest discover -s scripts/tests` завершаются успешно.
- Обычные Markdown links проверены отдельно от wikilinks; новые ссылки разрешаются, существующая история не искажена.
- `scripts/stride/build-game.ps1` создаёт новый Release ZIP и проходит Core-сценарии без старых ресурсов.
- `scripts/stride/verify-game.ps1 -PackageZip <новый ZIP>` проходит executable сценарии вне checkout, включая ресурсы, два разрешения и body traversal.
- При закрытии `scripts/stride/build.ps1` пересобирает полный Release Game Studio; в evidence записаны пути ZIP, игры, редактора и ограничения наблюдений.
- Локальная очистка имеет список удалённых/сохранённых путей, размер освобождённого места и расположение сохранённых отчётов.

## Порядок просмотра результата

- Текущие инструкции и назначение проекта: [README](../README.md).
- Исторические документы и восстановление исходников: [архив](archive/cpp/README.md).
- Сборки, проверка пакета, очистка диска и независимое ревью: [итоговый отчёт](audits/2026-09-09-cpp-retirement.md).
