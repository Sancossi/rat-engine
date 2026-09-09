# Очистка прежней C++-итерации — 2026-09-09

По [утверждённому плану](../cpp-retirement-plan.md) прежний C++-движок, редактор,
тесты, CMake, ресурсы и вспомогательные сборочные скрипты удалены из текущего
проекта. Stride/C# остался действующей реализацией. Статус работы хранится в
[карточке](../../vault/production/tasks/chore-retire-cpp-iteration.md).

## Изменения и история

- `c4540cb`: удаление реализации и C++ jobs CI, архив документации, обновление
  README/AGENTS/Cursor/BMad/проектных навыков, спецификаций и происхождения ресурсов.
- `74d5a57`: исправление замечаний ревью и навигационных wikilinks в архиве.
- Старые исходники доступны в Git на `60fdc55f19889304fc29957fd2124624077aefcd`;
  [архив](../archive/cpp/README.md) объясняет восстановление и исторические пути.
- PNG, Noto Sans и OFL внутри Stride сохранены побайтово; генератор остаётся рядом
  с игрой. Игровой код, сцены, версии зависимостей и upstream Stride не изменены.
- Одновременный внешний коммит `bce0c84` по Zed сохранён; он не является частью
  очистки. Очередь Sprint 19 не продолжалась, закрытые карточки не открывались.

## Локальная очистка

[Manifest](../../build/cpp-history/cleanup-manifest.json) и подробные списки
`removed-paths.json`, `preserved-files.json`, `file-inventory-before.json` находятся
в `build/cpp-history`. Эти локальные файлы игнорируются Git.

Удалены 39 явно проверенных путей: 8 438 825 296 байт старых сборок, пакетов,
clangd-кэшей и Qt-лога. До удаления сохранены 5 055 файлов общим размером
2 901 097 806 байт: отчёты, снимки, fixtures и прочие материалы. Копии проверены
по SHA-256; независимый reviewer повторно сверил все сохранённые файлы. Ведущий
проверил размеры/наличие всех копий и отсутствие всех удалённых корней.

Чистое уменьшение суммы размеров файлов: **5 537 727 490 байт (5,54 ГБ / 5,16 GiB)**.
Это учёт длин файлов за вычетом сохранённых копий, без оценки кластеров файловой
системы и размера новых manifest. Основной объём сохранённого архива — снимки.
Все `build/stride-game`, `build/tools`, игровые caches/settings, `.superpowers`,
общие инструменты и соседний Stride сохранены; неизвестные пути вне явного списка
не удалялись.

## Проверки

| Проверка | Результат |
|---|---|
| `python scripts/check_vault.py` | Passed |
| `python -m unittest discover -s scripts/tests` на Windows | 22 passed |
| actionlint для оставшегося CI workflow | Passed |
| `git diff --check` | Passed |
| Проверка навигации после исправлений | 274 локальных Markdown targets, 3 исходных targets в Git baseline, 26 архивных wikilinks; missing: none |
| `scripts/stride/build-game.ps1` | Release ZIP создан, 28 Core passed |
| `scripts/stride/verify-game.ps1 -PackageZip <новый ZIP>` | 17 executable сценариев passed |

Повторяемая локальная проверка ссылок:
`python build/cpp-history/check_cpp_retirement_links.py`; отчёт сохраняется в
`build/cpp-history/link-check.json`. Количество ссылок увеличивается при добавлении
этого итогового отчёта; приведённые выше числа относятся к проверенному implementation.

Игра: [Release executable](../../build/stride-game/20260909-043319-178/publish/Rat.Expedition.Windows.exe).
Пакет: [ZIP](../../build/stride-game/20260909-043319-178/rat-expedition-0.1.0-win-x64.zip).
[Build manifest](../../build/stride-game/20260909-043319-178/publish/build-manifest.json)
закреплён на `57cfae70a1e8a5e81fd1f2eaf844845e27bf3e4a`, `gameWorkingTreeDirty=false`;
последующие коммиты изменяют только документацию и статусы.

Изолированная проверка извлечённого ZIP:
[verification.json](../../../rat-expedition-validation/20260909-043349-746/verification.json).
Проверены 720p/1080p, границы камеры и zoom, лаз/лестница, large-Y startup и девять
ожидаемых отказов повреждённых/отсутствующих ресурсов и некорректных сцен.
Ведущий просмотрел реальные GPU-кадры `body-blocked-stand.png` и
`body-climb-up.png` в `body-1920x1080`: видны герой, геометрия и русские подсказки.

Release Game Studio собран существующим `scripts/stride/build.ps1`, без изменения
upstream. Первый запуск завершился ошибкой MSB3231: временно занят
`SpriteStudioAnimation.png` в глобальном NuGet-кэше. Повторный запуск прошёл:
[result.json](../../../stride/logs/rat-foundation/20260909-043132-584/result.json),
0 ошибок, 5 предупреждений upstream NU5100 об упаковке native template assets.
SDK 10.0.300, движок `e2c786a45f69917bf233793f6a097b150e2fe264`, версия 4.4.0-dev.
Редактор: [Stride.GameStudio.exe](../../../stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe).
Финальная пересборка после закрытия карточки записывается дополнением ниже.

## Независимое ревью

Три read-only прохода: blind diff, edge/deletion и acceptance. Найдены три
документационных замечания: ссылка исторического отчёта на новый CI-контракт,
приписывание Ubuntu CI Windows-only checkout guards и устаревшее описание
сохранённого C++-кода в действующем source workflow. Все исправлены в `74d5a57`.
Повторные edge/deletion и acceptance проверки: **Approved**. Acceptance reviewer
также подтвердил все SHA-256 сохранённого локального архива.

## Ограничения и Bugs found

Новых runtime-дефектов: **none**. Замечания ревью исправлены в рамках этой карточки.
Проверки выполнены на текущем Windows ПК с реальным GPU; ручной ввод, чистая машина,
интерактивный Game Studio и remote CI не заявляются проверенными. Игровая сборка
имела предупреждение CS0162 из генерируемого Stride кода; проверку это не остановило.
