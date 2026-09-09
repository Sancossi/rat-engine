# A1.2 — native карты, Core и MCP

[Контракт](../stride-native-map-authoring-spec.md), [карточка A1](../../vault/production/tasks/feat-stride-game-studio-authoring.md).
Работа продолжена в `C:/5_gamedev/rat-engine`, `feat/stride-native-authoring`, после
принятого MCP и baseline `28ef65d`. Этот срез переносит две карты, не библиотеку
ресурсов A1.3 и не отложенную ручную приёмку P1. Engine integration остаётся
`88301e861149c48c8b408aac3030b190332d7f97`, пакеты `4.4.0-dev` из прежнего locked cohort.

## Реализация

`Rat.Expedition.sln` открывает игровой Windows проект и Authoring assembly.
`ExpeditionProject.sdscene` с native UrlReference связывает Courtyard/Sluice.
Типизированные компоненты задают box/ramp, default/named spawns, ladder points,
portals, recovery и occlusion. Identity — Entity.Id, подпись — только display name.
Core остаётся без Stride dependency; `DefaultSpawnId` заменяет обязательное имя
`entry` для native карт, сохраняя прежний default для исторических JSON tests.

Один NativeSceneAdapter валидирует authored transforms/geometry/references и
создаёт чистые Core values. GeometryPreviewProcessor работает только в editor
ExecutionMode.Editor, обновляя generated mesh из тех же размеров. Ramp mesh
вынесен из runtime в общий FiniteRampPrimitive без изменения геометрии/winding.
Native данные каждого кандидата загружаются через отдельный ContentManager и
выгружаются после конверсии; активный renderer по-прежнему владеет своими ресурсами.
Пересборка уже открытого bundle не является обещанным hot reload.

Фактический pinned `Prefab.Instantiate()` сохраняет GUID; узкий game-owned wrapper
назначает новые entity ids после штатного clone. Проверены непересекающиеся id
двух экземпляров и remapped внутренние Entity references. Библиотека prefab и
редакторское редактирование instance overrides остаются A1.3.

## Настоящая правка стены через MCP

Owned editor PID 40632, `build/stride-mcp/connection.json`. Использован официальный
Python MCP SDK 2.2 через stdio server; запросы записаны в JSON. Mouse/keyboard input
не использовался. Scene `a6d301f9-3171-5137-b3c7-4622618bebc5`, wall entity
`64e053ae-e44a-530f-a63a-824bf7c62343`, Transform component
`6923d20e-7169-53b6-9d69-775824e72e98`.

| Шаг | Доказательство под `build/a12-wall-mcp/` |
| --- | --- |
| Position `(0,.9,0)` → `(0,.9,-1.5)`, Undo/Redo, Save, close/reopen, inspect | `shift/calls.json`, `shift/result.json`, 37 вызовов |
| Вид сдвинутой стены в editor | `shift/viewport-37.png` |
| Compile и actual runtime, exit 0 | `shift/runtime/native-world.json`, `shift/runtime/frame-0030.png` |
| Возврат `(0,.9,0)` через API с Undo/Redo/Save/reopen | `restore/calls.json`, `restore/result.json`, 32 вызова |
| Восстановленный вид editor/runtime | `restore/viewport-32.png`, `restore/runtime/frame-0030.png` |
| Восстановленный compiled Core | `restore/runtime/native-world.json`, exit 0 |

Core SweepFraction для полос X −3→3, radius .2, height .8: после сдвига старая
полоса Z=0 свободна (`1`), новая Z=−1.5 блокирует (`0.13333333`). После восстановления
результаты обратные. HasClearance в центрах полос также меняется true/false →
false/true. Это реальные запросы к compiled native world, не вычисление из PNG.
Wall GUID сохраняется. Финальная исходная карта содержит восстановленную геометрию.

Команды этого эксперимента: Python helper `build/a12-wall-api.py` (`--restore` для
возврата), `dotnet build games/rat-expedition/Rat.Expedition.Windows -c Release
--no-restore`, затем actual exe `--smoke-route zoom --smoke-frames 60 --evidence-dir
<каталог>`, cwd `%TEMP%`. Helper и записи — локальные evidence artifacts; публичный
универсальный клиент находится в `tools/stride-mcp/client.py`.
Первый `shift/viewport-5.png` снят до готового кадра и оказался чёрным: он исключён
из визуальной приёмки. Parent осмотрел shift37 и restore32, а также runtime frame30.

Parent независимо сравнил восстановленный native export с исторической JSON
геометрией: обе сцены, ссылки/списки и 146 численных полей, max abs delta `2e-7`,
errors `[]`: `build/a12-parent/restored-native-parity.json`. JSON использован только
как сравнительный baseline; новый runtime и ZIP его не читают.

## Проверки перед runtime commit

- Core 63 passed; Authoring 14 passed. Последний adapter log —
  `build/a12-adapter-tests.log`. Session regression выполняет обычное движение к
  порталу и E, затем наблюдает source failure: прежние Scene/position/stance/
  safe point/party, revision 0 и единственный initial renderer prepare/activate.
- Python 25 passed, `build/a12-python-tests.log`; vault/diff checks passed.
  Два новых Python случая покрывают harmless entity rename в генераторе и
  сохранение authored root list вместо превращения каждого child в root.
- Preview package `build/stride-game/20260909-121457-086`: Core/adapter/publish
  прошли. Это WIP, manifest честно отмечает dirty; не финальный пакет.
- Preview verifier `C:/5_gamedev/rat-expedition-validation/20260909-121618-241`
  прошёл 4 OS states (~59–60 Hz), walls 720/1080, edge zoom 4.5/7, wheel, body
  720/1080. Затем выявил ошибку QA setup: large-Y platform имела неверные XZ при
  сохранённых исходных ladder points. Исправлен только fixture, physics не менялась.
- Узкий повтор после исправления: `build/a12-native-preview2/results.json`,
  16 expected outcomes — large-Y, три recovery, одиннадцать native startup failures
  и portal→invalid compiled native candidate. Все прошли. Recovery/native failure
  завершили реальный маршрут, а не только старт приложения.

Стартовый новый session test сначала корректно отвергал fixture со spawn внутри
портала; setup исправлен движением к отдельно стоящему порталу. Первый rename test
поймал зависимость QA box cloning от старой подписи; генератор исправлен на GUID.

## Пакеты и границы

Обычный build включает production native assets. Полная проверка использует
`scripts/stride/build-game.ps1 -IncludeQualificationAssets`: отдельный derived
native package в `obj/native-qualification`, тот же compiler/root production maps.
QA fixtures не редактируют исходные карты и не являются вторым manual JSON.
Обычная сборка не зависит от тестовых имён/геометрии. Полный verifier требует
проверочный ZIP; `--native-project QA/...` запрещён в обычном запуске.
Mapping старых 33 сценариев и трёх добавленных описан в контракте; ожидается 36.
Финальные committed-head artifact/review результаты добавляются отдельно после
этой предварительной квалификации.

Не заявлены: clean-machine запуск, ручной ввод/ощущение движения, новый F5 playtest
в этом срезе, live bundle hot reload, полный native asset catalog, mesh collision,
GUI rename/delete всех категорий. Legacy JSON остаётся в исходниках для Core tests,
но исключён из publish. Статусы, финальную Release editor сборку и публикацию
принятого среза в main ведёт parent; незавершённые A1.3/A1.4 этим отчётом не закрываются.

## Independent review: два исправления

`857e3a1` прошёл все 36 executable сценариев:
`C:/5_gamedev/rat-expedition-validation/20260909-122643-970/verification.json`.
Пакет `build/stride-game/20260909-122553-348/rat-expedition-0.1.0-win-x64.zip`,
SHA256 `7816C55A55152201A9FC108DA0AC7E9464C5BC2CE60BBE6F9D55D7C87658F3ED`,
gameCommit `857e3a1`, dirty=false. Обычный пакет без QA также собран в `122654-343`
(manifest `59fa6ac`, только parent card change поверх того же кода, dirty=false),
но эти сборки предшествуют следующим review fixes и не заменяют их проверку.

1. Reviewer воспроизвёл ложный floating wall при общем parent Y=.1: floor top
   `.10000000149`, wall bottom `.10000002384`. Новый regression до исправления:
   `build/a12-vertical-parent-before.log`, 14 pass / 1 fail. Adapter теперь складывает
   parent translations и вычисляет bounds в double, округляя только окончательный
   результат. Core epsilon/точное требование wall-floor не менялись. После исправления
   `build/a12-vertical-parent-after.log`: 15 passed; parent Y=+.1/−.1/.3/1000 сохраняет
   support/contact, настоящая добавка +.01 к стене отвергается.
2. Preview.Draw раньше имел Order=0, равный ModelRenderProcessor. OrderedCollection
   не гарантирует порядок равных элементов; прежние buffers могли уже попасть в
   текущий RenderMesh. Теперь preview Order=−300, до TransformProcessor(−200) и
   ModelRenderProcessor(0). Source anchors integration 88301e8:
   `sources/engine/Stride.Engine/Engine/EntityManager.cs:199`,
   `Engine/EntityProcessorCollection.cs:59`, `Engine/Processors/TransformProcessor.cs:46`,
   `Engine/Processors/ModelTransformProcessor.cs:35`, `Rendering/ModelRenderProcessor.cs:75`.
   Модель создаётся до обновления skeleton/world matrices и renderer collection.

После прежних builds editor показывал пять dirty scenes. Native history через
read-only UIAutomation подтвердил только четыре `Reload game assemblies` после
двух наших MCP Position transactions: `build/a12-history-open-uia.json`.
Последний сохранённый revision был 12, reload довёл его до 16. Известное reload-state
сохранено через MCP (`build/a12-save-reload.json`), затем old owned PID 40632 закрыт
штатно. Save канонизировал только Sluice: порядок entities, представление float и
явный default white LightAmbient; значения gameplay не изменились. Неизвестные
ручные правки не отбрасывались.

Свежий owned editor PID 45016 запущен через `start-mcp-editor.ps1 -SolutionPath
C:/5_gamedev/rat-engine/games/rat-expedition/Rat.Expedition.sln`; Debug preflight
собрал исправленную assembly (SHA256 `ba5aca019747ec37be796555e7223262991ddc2606f165ee0698259de093ba19`).
Квалификация `build/a12-preview-lifecycle/evidence.json` прошла 85 MCP вызовов:
13 Size/Role изменений, Undo/Redo, Save/close/reopen и возврат исходных Size/Role.
17 captures ожидали реальный editor NextFrame, имели непустые pixels, 7 различных
hash; diagnostics errorCount=0. PNG не сохранялись и не использовались для управления.
Это проверка работающего render/замены моделей, не ручной осмотр формы или GPU
memory profiling. Позиция/размер/роль исходной стены восстановлены и сохранены.

## Исправленный committed-head build

Оба P2 исправлены в `49a9923`; повторное независимое review — Approved, новых
блокеров не найдено. Перед сборками MCP подтвердил revision 30, dirtyAssets=[],
busy=false (`build/a12-fixed-pre-close.json`), после чего owned PID 45016 закрыт
штатным CloseMainWindow. Чужие сессии не закрывались.

Parent выполнил полную Release сборку Game Studio: exit 0, 64.16 s, пять известных
NU5100 warnings, 0 errors. Результат:
`C:/5_gamedev/stride/logs/rat-foundation/20260909-125243-034/result.json`.
Редактор: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
Integration SHA остаётся `88301e861149c48c8b408aac3030b190332d7f97`.

Затем последовательно выполнены `build-game.ps1 -IncludeQualificationAssets` и
`build-game.ps1`: оба exit 0, Core 63 / authoring 15. Manifest обоих пакетов фиксирует
`b403f84dd35db7ff67730387aecd59be7b2d1a5f` (поверх исправления только parent card),
gameWorkingTreeDirty=false, self-contained win-x64. Посторонние EOL изменения vault
сохранены; чистота всего repository не заявляется.

| Пакет | Путь относительно repository | SHA256 |
|---|---|---|
| QA, qualificationAssets=true | `build/stride-game/20260909-125416-102/rat-expedition-0.1.0-win-x64.zip` | `56E9B7F94A552E72AB94D09470351C8747A31CA7684998FA0FCD4901FDEF4C30` |
| Обычный, qualificationAssets=false | `build/stride-game/20260909-125515-649/rat-expedition-0.1.0-win-x64.zip` | `4E18ABCC41A58EAB7489E9C6429208C5AACC7348A350C29FAC0FD4BB639BE59B` |

Логи сборок: `build/a12-package-fixed.log`, `build/a12-package-production-fixed.log`.

`verify-game.ps1 -PackageZip <QA ZIP>` завершился exit 0: **36 сценариев**, из них
21 успешный запуск и 15 ожидаемых отказов. Итоговый файл:
`C:/5_gamedev/rat-expedition-validation/20260909-125458-633/verification.json`;
лог `build/a12-verifier-fixed.log`. Проверены обе размерности окна, ramp/bridge
верх/низ, local occlusion и mixed companions, десять portal roundtrips, сохранение
active world при renderer/native candidate failure, три recovery слоя, large-Y
startup, трансформации/ссылки и отказы ресурсов. Четыре фактических OS window states
дали steady 59.30–59.68 Hz. Это автоматические команды/реальный GPU, не ручной ввод.

Parent независимо сравнил исправленный compiled native export с историческим
baseline: `build/a12-parent/fixed-native-parity.json`, две сцены, 146 численных
полей, max absolute delta 2e-7, errors=[]; сравнивались также разрешённые по именам
references/lists. Старый JSON используется только как исторический эталон проверки.

Обычный production ZIP отдельно распакован и запущен с `--smoke-route zoom
--smoke-frames 60` из чужого cwd. Exit 0, реальный кадр и две native сцены загружены;
полный `native-world.json` совпал с production world проверочного ZIP. Legacy
`courtyard.json`/`sluice.json`/`project.json` отсутствуют в опубликованном Content.
Результат: `C:/5_gamedev/rat-expedition-validation/a12-production-20260909-125515-649/result.json`;
exe: `C:/5_gamedev/rat-expedition-validation/a12-production-20260909-125515-649/extracted/Rat.Expedition.Windows.exe`.
Это standalone smoke на машине разработки, не clean-machine или новый manual playtest.

Vault/diff и синхронизированная BMad projection проверены. A1.2 завершает миграцию
двух карт; A1.3 resource catalog/import и A1.4 общая authoring приёмка остаются
следующими срезами. MCP используется через structured status/inspect/property/save;
viewport снимки не нужны для обычного управления. Финальные статусы/публикация —
в канонической карточке, не в этом audit.
