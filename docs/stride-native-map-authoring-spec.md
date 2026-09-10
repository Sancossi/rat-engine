# A1.2 — native карты и Core

Срез [A1](stride-editor-asset-workflow-plan.md) переносит фактические Courtyard и Sluice в `Rat.Expedition.Authoring/Assets`. Статусы ведёт [карточка A1](../vault/production/tasks/feat-stride-game-studio-authoring.md).

## Авторский источник

`ExpeditionProject.sdscene` содержит ссылки `UrlReference<Scene>` на стартовую сцену и каталог. Карты содержат типизированные компоненты геометрии, spawn, лестниц, порталов, восстановления и occlusion. Native Entity.Id — игровой id; display name служит только понятной подписью и диагностическим экспортом. DefaultSpawn — явная локальная Entity reference. Портал ссылается на native scene asset; пустой TargetSpawnId означает её DefaultSpawn, иначе это GUID конкретной spawn entity. Rename не меняет GUID. Удалённые/чужие ссылки и дубли id отвергаются. Локальные ladder/occlusion references должны сохранять native remapping при clone; prefab library остаётся A1.3.

Box: Entity.Position — центр, Size — конечные положительные размеры. Ramp: Entity.Position — начальный угол верхней поверхности, Size.X/Z — горизонтальные пролёты, Size.Y — толщина, Rise — signed перепад вдоль Axis. Используется прежняя конечная slab geometry с вертикальными торцами, без коллизии произвольного mesh. Для геометрии/точек и родителей допустима только конечная трансляция, identity rotation, unit scale и TRS. Изменение Size/Position обновляет один источник геометрии. Editor processor показывает box/точную ramp; runtime строит прежнюю owned presentation из того же validated Core snapshot. Это не два вручную поддерживаемых набора размеров.

Сумма parent translations и centre±Size/2 вычисляется с double intermediates;
float округляются только итоговые bounds/points. Поэтому общий parent Y=.1 не
создаёт искусственный зазор floor/wall. Строгое Core правило контакта не ослабляется:
реально поднятая стена по-прежнему требует Role=Structure. Preview generation имеет
Order −300, до TransformProcessor(−200) и ModelRenderProcessor(0), чтобы новая
геометрия получила world matrices, а старые буферы не освобождались после сбора
RenderMeshes текущего кадра.

Core остаётся без Stride references. Native adapter проверяет metadata, затем прежние Core clearance/support/corridor/reference guards. Каждый candidate заново читает native assets через отдельный ContentManager, конвертирует в plain values и выгружает native scene; renderer подготавливается до атомарной смены session. Ошибка candidate оставляет старый мир. Активный loader не использует старый JSON как fallback. Legacy JSON до A1.4 может оставаться для исторических Core fixtures; производные native-world.json являются только evidence.

## Приёмка

- Native compiler/load фактических двух сцен, численная parity с прежней геометрией, все 63 Core cases и эквивалентные executable regression scenarios.
- Invalid dimensions/rotation/scale/parent transform, duplicate identity, отсутствующие spawn/ladder/portal/occlusion references дают контекст asset/entity/property. Переименование не ломает identity; дублирование даёт новые native ids и корректные локальные ссылки.
- Реальный GameStudio: открыть Courtyard, переместить стену, сохранить/переоткрыть. Новый compiled runtime показывает сдвиг; Core sweep свободен на старом месте и блокируется на новом. Проверить прежнюю сцену на ошибке кандидата.
- Сохранены FSM/feet Y, слой моста, true-3D trail, партия .7/1.4, zoom/focus/pause и 60 Hz smoke policy. Audio/UI/native sprite catalog — следующий A1.3, не работа этого среза.

`build/stride-game/*/native-world.json` экспортирует валидированные сцены и отдельный GUID→DisplayNames для независимой проверки. Состояния/occlusion используют GUID; подписи в evidence не участвуют в игровом выборе.

Квалификация обнаружила: pinned runtime `Prefab.Instantiate()` сохраняет entity GUID. Для runtime экземпляров добавлен `NativePrefabInstances.Instantiate`: сначала штатный clone с remapping объектных ссылок, затем новые GUID каждому entity. Editor duplicate использует свой GenerateNewIds. Portal.TargetSpawnId обозначает внешний spawn и не переназначается при копировании портала. Полная библиотека prefab/instances остаётся A1.3.

Fresh ContentManager устраняет cache загруженных объектов, но не обещает hot reload уже открытого compiled database/bundle. Сохранённые GUI изменения применяются после compile и нового запуска; каждый transition заново загружает и валидирует данные текущего build.

## Проверочный пакет и границы

Обычный build упаковывает только production assets. `build-game.ps1 -IncludeQualificationAssets`
добавляет derived native fixtures в отдельный игнорируемый package под `obj/native-qualification`;
исходные sdscene не меняются. Генератор адресует объекты по GUID и сохраняет native RootParts,
не требует неизменных display names. QA manifest выбирается только явным smoke аргументом;
обычный запуск не переключается на тестовые данные. Полный verifier требует этот проверочный ZIP.

33 прежних сценария адаптированы: четыре OS window states, два walls GPU, два edge zoom,
wheel, два body, large-Y, четыре PNG/font отказа, девять native отказов вместо JSON-era
fixtures, два layered, mixed, portals, renderer failure и три recovery. JSON missing/malformed/
missing coordinate/project заменены native missing asset/invalid size/missing DefaultSpawn/
missing StartScene; ladder/ramp missing coordinate — отсутствующей Entity reference и
вырожденным native размером. Bad target/occlusion и blocked exit сохранены. Дополнительно
проверяются rotation, scale и native source candidate failure: итого ожидается 36 executable
сценариев. Это native compile/load/validation tests, не утверждение о проверке прежнего JSON parser.

Стенной эксперимент через MCP использует старую полосу `(−3,0,0)→(3,0,0)` и новую
`(−3,0,−1.5)→(3,0,−1.5)`; runtime экспортирует реальные Core SweepFraction/clearance
после compile. После проверки стена возвращается через API в исходную Position `(0,.9,0)`.

## Suggested Review Order

1. [Native компоненты](../games/rat-expedition/Rat.Expedition.Authoring/TraversalComponents.cs),
   [карты](../games/rat-expedition/Rat.Expedition.Authoring/Assets/ExpeditionProject.sdscene).
2. [Конверсия и валидация](../games/rat-expedition/Rat.Expedition.Authoring/NativeSceneAdapter.cs),
   [загрузка кандидата](../games/rat-expedition/Rat.Expedition.Authoring/NativeProjectLoader.cs).
3. [Editor preview](../games/rat-expedition/Rat.Expedition.Authoring/GeometryPreviewProcessor.cs),
   [runtime](../games/rat-expedition/Rat.Expedition.Windows/ExpeditionGame.cs).
4. [Adapter tests](../games/rat-expedition/Rat.Expedition.Authoring.Tests/Program.cs),
   [native fixtures](../games/rat-expedition/tools/build_native_fixtures.py),
   [build](../scripts/stride/build-game.ps1), [verifier](../scripts/stride/verify-game.ps1).

## Current Dremma production extension

The [Dremma integration contract](dremma-game-integration-spec.md) extends this
accepted A1.2 base. `ExpeditionProject.sdscene` now starts the real hand-authored
`Dremma.sdscene` and references Dremma, Courtyard, and Sluice. Dremma–Courtyard
is two-way; the existing Courtyard–Sluice IDs, order, and topology are retained.

Dremma authors the native bridge at metre scale, finite curved-arch collision,
a west end-on stair/ramp with solid masonry and sloped handrail guards, plaza,
portal, static door, four scale figures, canal wall, and lanterns. Bridge arch,
deck, and rails use separate native subsets and occlusion groups. A backward
compatible `AdditionalGeometryIds` collection lets one native subset replace
several explicit collision entities without duplicate editor previews. Runtime
still rejects missing, empty, duplicate, or foreign links before activation.

Current dimensions supersede older P1 examples: body 1.8/0.9 m and radius 0.45
m; camera default 11.25 and range 10.125–15.75. Speeds 3/1.5/1 m/s, gravity,
FSM, fixed tick, animation timing, input, and party spacing 0.7/1.4 m are
unchanged. Normal runtime loads compiled native assets only. The manual Dremma
scene is never generated by `tools/build_native_fixtures.py`.
