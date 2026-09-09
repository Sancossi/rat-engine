# A1.2 — native карты и Core

Срез [A1](stride-editor-asset-workflow-plan.md) переносит фактические Courtyard и Sluice в `Rat.Expedition.Authoring/Assets`. Статусы ведёт [карточка A1](../vault/production/tasks/feat-stride-game-studio-authoring.md).

## Авторский источник

`ExpeditionProject.sdscene` содержит ссылки `UrlReference<Scene>` на стартовую сцену и каталог. Карты содержат типизированные компоненты геометрии, spawn, лестниц, порталов, восстановления и occlusion. Native Entity.Id — игровой id; display name служит только понятной подписью и диагностическим экспортом. DefaultSpawn — явная локальная Entity reference. Портал ссылается на native scene asset; пустой TargetSpawnId означает её DefaultSpawn, иначе это GUID конкретной spawn entity. Rename не меняет GUID. Удалённые/чужие ссылки и дубли id отвергаются. Локальные ladder/occlusion references должны сохранять native remapping при clone; prefab library остаётся A1.3.

Box: Entity.Position — центр, Size — конечные положительные размеры. Ramp: Entity.Position — начальный угол верхней поверхности, Size.X/Z — горизонтальные пролёты, Size.Y — толщина, Rise — signed перепад вдоль Axis. Используется прежняя конечная slab geometry с вертикальными торцами, без коллизии произвольного mesh. Для геометрии/точек и родителей допустима только конечная трансляция, identity rotation, unit scale и TRS. Изменение Size/Position обновляет один источник геометрии. Editor processor показывает box/точную ramp; runtime строит прежнюю owned presentation из того же validated Core snapshot. Это не два вручную поддерживаемых набора размеров.

Core остаётся без Stride references. Native adapter проверяет metadata, затем прежние Core clearance/support/corridor/reference guards. Каждый candidate заново читает native assets через отдельный ContentManager, конвертирует в plain values и выгружает native scene; renderer подготавливается до атомарной смены session. Ошибка candidate оставляет старый мир. Активный loader не использует старый JSON как fallback. Legacy JSON до A1.4 может оставаться для исторических Core fixtures; производные native-world.json являются только evidence.

## Приёмка

- Native compiler/load фактических двух сцен, численная parity с прежней геометрией, все 63 Core cases и эквивалентные executable regression scenarios.
- Invalid dimensions/rotation/scale/parent transform, duplicate identity, отсутствующие spawn/ladder/portal/occlusion references дают контекст asset/entity/property. Переименование не ломает identity; дублирование даёт новые native ids и корректные локальные ссылки.
- Реальный GameStudio: открыть Courtyard, переместить стену, сохранить/переоткрыть. Новый compiled runtime показывает сдвиг; Core sweep свободен на старом месте и блокируется на новом. Проверить прежнюю сцену на ошибке кандидата.
- Сохранены FSM/feet Y, слой моста, true-3D trail, партия .7/1.4, zoom/focus/pause и 60 Hz smoke policy. Audio/UI/native sprite catalog — следующий A1.3, не работа этого среза.

`build/stride-game/*/native-world.json` экспортирует валидированные сцены и отдельный GUID→DisplayNames для независимой проверки. Состояния/occlusion используют GUID; подписи в evidence не участвуют в игровом выборе.

Квалификация обнаружила: pinned runtime `Prefab.Instantiate()` сохраняет entity GUID. Для runtime экземпляров добавлен `NativePrefabInstances.Instantiate`: сначала штатный clone с remapping объектных ссылок, затем новые GUID каждому entity. Editor duplicate использует свой GenerateNewIds. Portal.TargetSpawnId обозначает внешний spawn и не переназначается при копировании портала. Полная библиотека prefab/instances остаётся A1.3.

Fresh ContentManager устраняет cache загруженных объектов, но не обещает hot reload уже открытого compiled database/bundle. Сохранённые GUI изменения применяются после compile и нового запуска; каждый transition заново загружает и валидирует данные текущего build.
