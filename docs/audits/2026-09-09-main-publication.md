# Подготовка принятой версии к main

Подготовка в `C:/5_gamedev/rat-engine-mcp`, ветка `publish/accepted-main`.
Карточка: [публикация проекта и редактора](../../vault/production/tasks/publish-accepted-project-editor-main.md).
Этот отчёт описывает локальное слияние; remote publication и default branches
проверяет parent после независимого review.

## Истории и разрешение

- Принятая версия: `ad733e23756f1c2b649646ac3aebb3cb83600a40`.
- Первый родитель с карточкой публикации: `aaad3455978127500d7ac125687ad5797db10987`.
- Присоединяемый `origin/main`: `71299028f713b3bd6a1d5fe09faa7fe7a14d590d`.
- Общая база: `056fc74186a0f93c7357dc71857031de05cbb65f`.
- До merge: 625 коммитов только в принятой ветке и 5 только в main.
  Main-only: `3f1ba85`, `99d77ab`, `afa1db8`, `d3ba0d3`, `7129902`.

Выполнен обычный `git merge --no-commit origin/main`, без переписывания истории.
Текстовых конфликтов нет. Единственный итоговый main-only файл — первоначальный
321-строчный `docs/architecture-roadmap.md`. Возвращать его как текущий C++ план
после retirement было бы семантическим конфликтом. Полный текст перенесён в
[отдельный архив](../archive/cpp/architecture-roadmap-main-7129902.md); blob
`5fd20d36c848adba36888e6967595a3d0c543809` совпадает с исходным main.
Поздняя архивная редакция сохранена. [Старый URL](../architecture-roadmap.md)
ведёт к текущей архитектуре Stride и обеим историческим версиям.

## Точный объём относительно принятой версии

От `ad733e2` изменяются только семь документов: три parent-owned карточки
(`feat-stride-editor-mcp`, `publish-accepted-project-editor-main`,
`publish-editor-game-vault-snapshot`), новый указатель `docs/architecture-roadmap.md`,
архивный оригинал, `docs/archive/cpp/README.md` и этот отчёт.
Статусы карточек implementer не менял.

Деревья игры, MCP/engine lock и скриптов совпадают с `ad733e2`:

| Каталог | Git tree |
| --- | --- |
| `games` | `b0c7de9408bfca941450d4c8ca4dd1e258ea9f11` |
| `tools` | `91b61b878260111099f8e89f35a629c766e94449` |
| `scripts` | `012a117f2f54e7302a46fcf576f57b84fd2b78fa` |

Исходный checkout с 24 незавершёнными A1.2 файлами не редактировался; parent
отдельно подтвердил неизменность их SHA256. Это слияние не принимает A1.2 или
оставшуюся ручную проверку P1. Исходники Stride не изменялись: upstream baseline
`e2c786a45f69917bf233793f6a097b150e2fe264`, integration
`88301e861149c48c8b408aac3030b190332d7f97`. План публикации fork main — из этой
интеграции, без 16 более новых upstream-коммитов fork master и без изменения pin.

## Проверки

Перед commit: `git diff --check`, `git diff --cached --check`,
`python scripts/check_vault.py`, `python scripts/bmad_vault.py sync` и `check`.
Все проверки прошли: vault valid, projection Sprint 19 совпадает с authoritative
vault, whitespace errors отсутствуют. Новые runtime тесты для этого
документального слияния не запускались: указанные деревья совпадают.

Parent выполнил полную Release сборку редактора: повторная попытка
`C:/5_gamedev/stride/logs/rat-foundation/20260909-114644-943/result.json`,
exit 0, 44.20 s, 5 upstream NU5100 warnings, 0 errors. Editor:
`C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
Первая попытка `20260909-114512-725` завершилась MSB3231 из-за файла
`SpriteStudioAnimation.png`, удерживаемого owned editor PID 66276; parent проверил
чистую сессию и штатно закрыл её перед повтором. Это окружение сборки, не изменение
кода. GUI и remote CI этой сборкой не квалифицируются.

## Публикация и финальная проверка parent

Независимое read-only review одобрило merge `1c344ed` / HEAD `9b3f9ab` и план
публикации. Parent подтвердил ancestor checks для старого main и `ad733e2`,
равенство всех трёх trees из таблицы и повторный vault check.

Обычный push обновил [rat-engine/main](https://github.com/Sancossi/rat-engine/tree/main)
с `7129902` до `9b3f9ab7f17fefb0121ec619d55a03e3ac117eb0` перед этой документальной
closure. [Stride/main](https://github.com/Sancossi/stride/tree/main) создан на
`88301e861149c48c8b408aac3030b190332d7f97`; через GitHub API выставлен default main.
API и `git ls-remote --symref origin HEAD refs/heads/main` подтвердили main и
соответствующий точный SHA в обоих репозиториях. Fork master по-прежнему
`8781609b6a2b8fe61397a7baa7a4aaa79ec24e91`. Force push, переписывание upstream,
обновление engine pin или принятие новых upstream-коммитов не выполнялись.

После сборки parent снова открыл исходный проект штатным MCP launcher:
`C:/5_gamedev/rat-engine/build/stride-mcp/sessions/20260909-115129-222/connection.json`,
PID40632, ready. SHA256 всех 24 сохранённых WIP файлов совпали с исходным снимком
`build/mcp-parent-client/original-wip-before-merge.json`. Editor readiness не
объявляется новой GUI/gameplay квалификацией. Оставшиеся A1/P1 acceptance не меняются.
