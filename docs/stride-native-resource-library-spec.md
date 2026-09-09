# A1.3 — native библиотека ресурсов

Дата: 2026-09-09. [Карточка A1](../vault/production/tasks/feat-stride-game-studio-authoring.md),
[полный план](stride-editor-asset-workflow-plan.md), [принятые карты A1.2](stride-native-map-authoring-spec.md).
Вход: published A1.2 `ff24f314e3d5b3f08f955757b6516c301be65ff4`; Core 63,
authoring 15, executable 36. Статусы остаются в vault. Этот документ описывает
следующую реализацию, а source inspection ниже не считается live qualification.

Первый ограниченный API-срез: [фактическая квалификация](audits/2026-09-09-stride-resource-api.md).
Он включает catalog/inspect, typed scalar/reference edits и native source-update
lifecycle. Остальные команды и runtime library ниже остаются последующими частями
этого контракта; наличие source anchors не означает их реализации.

## Результат и сохраняемые границы

По одному собственному примеру prefab, imported mesh, texture/material, sprite
sheet, sound и UI/font открывается и редактируется в Game Studio, используется
обычной игрой и попадает в self-contained ZIP. Отдельная библиотечная сцена допустима
для preview, но два пропа также размещаются в настоящем Courtyard: runtime обязан
учитывать native Model/Material и transform из карты. Нет второго JSON со ссылками
или копии параметров material/atlas в C#.

Core/FSM, столкновения, feet Y, ramp/bridge, партия .7/1.4, камера 5.0 и smoke 60 Hz
не меняются ради ресурсов. Новые боевые, музыкальные, UI-навигационные системы,
три новых облика персонажей, универсальный редактор ресурсов и A1.4 вне среза.
VFX N/A: текущая игра не использует particles/effects. Mesh animation clips также
N/A для статического пропа; кадровая анимация существующей крысы обязательна.

## Источник → editor → runtime

Native definitions хранятся под `Rat.Expedition.Authoring/Assets/Library/`, исходники
под `Rat.Expedition.Authoring/Resources/Library/`: собственный небольшой OBJ с UV,
PNG, WAV и исходный генератор; существующие rat PNG и Noto TTF/OFL сохраняют
происхождение. Выбирается один канонический исходный путь каждого ресурса, а не две
редактируемые копии. При переносе PNG меняются также output генератора
`tools/rat_sprite.py` и `Content/credits.json` (source/exported path); старый активный
output не остаётся вторым источником. Credits и необходимые notices входят в ZIP. Mesh/UV, пиксели,
синтез звука редактируются внешним инструментом/исходным генератором; Game Studio
импортирует, размещает и меняет native свойства/ссылки.

Проект получает один сериализуемый native resource catalog с типизированными
content references для sheet/font/UI/sound. Это editor asset data, не ручной runtime
manifest. Ссылки разрешаются по native asset ID/type, URL сохраняется нативным
механизмом. Reference rename должен сохранять связь; deletion даёт диагностируемый
отказ и Undo восстанавливает её. Игровая идентичность объектов остаётся Entity.Id.

`NativeProjectLoader` продолжает возвращать только валидированные Core definitions.
Новая Windows-side resource ownership отдельно держит ContentManager/загруженные
assets. Native декоративные entities переносятся в подготовленный scene bundle с
их иерархией/transform, без изменения Core geometry. Generated collision preview
не дублируется как вторая модель. Весь fallible load/validation/clone выполняется
до transactional Activate. Active bundle сохраняется при ошибке, предыдущие leases
освобождаются после retirement; borrowed model buffers не Dispose вручную. Shared
sheet/font/sound принадлежат game lifetime, SoundInstance освобождается раньше Sound.
UIPage с изменяемым деревом требует отдельного instance/clone там, где есть несколько
владельцев. Fresh ContentManager не объявляется hot reload открытого database bundle.

## Проверенные в исходниках точки

Все пути ниже относительно `C:/5_gamedev/stride`, integration
`88301e861149c48c8b408aac3030b190332d7f97`, upstream `e2c786a45f69917bf233793f6a097b150e2fe264`.
Точные вызовы ещё нужно скомпилировать и проверить на собственных fixtures.

| Категория | Source anchor и подтверждённая граница |
|---|---|
| Prefab | `sources/engine/Stride.Assets/Entities/PrefabAsset.cs:47` — `CreatePrefabInstance` вызывает CreateDerivedAsset/id remapping. Editor `AddPrefabAssetPolicy.cs:37` использует этот путь. Он отличается от runtime `Prefab.Instantiate`, квалифицированного в A1.2 |
| Model/import | `sources/engine/Stride.Assets.Models/ThreeDAssetImporter.cs:18` поддерживает OBJ; `ModelAssetImporter.cs:77` импортирует AssetItems; `ModelAsset.cs:39` Source и `:83` Materials, расширение `.sdm3d` |
| Texture/material | `sources/engine/Stride.Assets/Textures/TextureAsset.cs:31` `.sdtex`; `Materials/MaterialAsset.cs:29` `.sdmat`, Attributes — native material graph |
| Sprite | `sources/engine/Stride.Assets/Sprite/SpriteSheetAsset.cs:37` `.sdsheet`, `:142` PremultiplyAlpha, `:164` Sprites; `SpriteInfo.cs:29/50/78` Source/TextureRegion/Center, PPU и CenterFromMiddle. `sources/engine/Stride.Engine/Rendering/Sprites/SpriteAnimationSystem.cs:109/148` умеет последовательности кадров |
| Sound | `sources/engine/Stride.Assets/Media/SoundAsset.cs:27` `.sdsnd`; `sources/engine/Stride.Audio/Sound.cs:43` CreateInstance, `SoundInstance.cs:175/234/339` ReadyToPlay/Play/PlayState |
| Font/UI | `sources/engine/Stride.Assets/SpriteFont/FileFontProvider.cs:29` локальный TTF; `OfflineRasterizedSpriteFontType.cs:51` CharacterRegions; `UI/UIPageAsset.cs:25` `.sduipage`, compiler `UIPageAssetCompiler.cs:42` RootElement; `sources/engine/Stride.UI/Engine/UIComponent.cs` Page/IsFullScreen/Resolution |
| Asset API | `sources/editor/Stride.Core.Assets.Editor/ViewModel/SessionViewModel.cs:105/1142` AllAssets/GetAssetById; `AssetViewModel.cs:200/166` PropertyGraph/Name; `PackageViewModel.cs:320/707` CreateAsset/explicit directory |
| Reimport/delete | `AssetSourcesViewModel.cs:76` public UpdateAssetFromSource(LoggerResult), native UI wrapper `:129` отдельно создаёт transaction; `AssetCollectionViewModel.cs:1415` DeleteContent |

## Узкое расширение MCP: сначала квалификация

Нынешние 12 tools адресуют сцены и простые component properties. Они не умеют
каталог, nested material/sprite/UI data, content references или import. Предлагаются
ограниченные команды catalog/inspect, typed property/reference edit, rename/delete,
import/reimport и prefab placement. Названия/объединение tools уточняются при первой
квалификации; capability response обязан честно перечислять поддержанные типы и ключи.

Read возвращает asset ID, тип, URL, dependencies, относительные sources и только
поддержанные поля. Edit принимает явный target asset/entity/item ID и ключ из allowlist:
например texture settings, sprite region/pivot, material color/texture, UI text/font,
native Model/Material reference. Никаких eval, произвольного reflection member path,
вставки C# или произвольного JSON graph. Prefab placement принимает конкретные scene/
prefab IDs и позицию, использует native hierarchy/base-part механизм и Undo.

Каждая мутация проверяет project/session, expectedRevision, тип и принадлежность
целевого ID редактируемому игровому package, native save/close/disposed state на
dispatcher. Wrong/stale ID, несоответствие reference type, недопустимые поля/числа
отклоняются до изменения. Rename/delete ограничены выбранными игровыми assets;
dependency diagnostics и Undo проверяются отдельно. Сохранение — существующий
session-wide Save с operation result, без переписывания `.sd*` вместо editor API.

Import требует **явного target package/directory**, исходника внутри game Resources
и allowlisted импортёра для fixture типов. `RunAssetTemplate` не использовать как
скрытое адресование: он зависит от current selection (`AssetCollectionViewModel.cs:490`).
Предпочтительный путь — native importer готовит AssetItems, затем public
PackageViewModel.CreateAsset вставляет их в явно разрешённую directory в Undo
transaction. Подготовка/валидация/cancel до commit не добавляют assets. Ошибка
посередине native insertion требует проверенного Undo rollback/transaction discard;
частичный import нельзя объявлять атомарным успехом. Generated IDs и внутренние
ссылки всего набора проверяются до вставки. Коллизия URL/ID не перезаписывает
существующий asset. Source dependencies
также проверяются, включая traversal, абсолютные внешние пути и reparse escapes;
проверка повторяется перед фактическим import/reimport.

Reimport public method сам не открывает transaction: адаптер создаёт её, держит
async busy operation и проверяет LoggerResult.HasErrors. Ошибка не считается успехом
по факту завершения Task; изменения либо откатываются, либо явно возвращается
диагностированный outcome без ложного clean state. Очередь до deadline не стартует
позже; начавшийся native import не объявляется отменённым задним числом. Нужны
конкурентные mutation/save/close тесты, включая native GUI операции: существующие
Save/Close сигналы не доказывают защиту нового import. Если public API не позволяет
обеспечить эту границу, остановить этот шаг и сообщить конкретный source blocker
до любого engine patch. Универсальный import browser/build-job API не добавлять.

## Fixture и acceptance matrix

| Fixture | Editor/API действие | Runtime/ZIP доказательство |
|---|---|---|
| Один prop prefab, два экземпляра Courtyard | Native instance IDs не пересекаются; изменить shared model/material, одну local override; Save/reopen, rename/Undo | Оба размещённых пропа видны, shared update действует на оба, local override остаётся только у своего; нулевая новая collision, валидные internal refs |
| Собственный UV mesh + PNG/material | Import, изменение OBJ/PNG source, reimport; смена native texture reference; Undo/Redo | Изменённые bounds/материал и реальный кадр до/после; missing/corrupt/outside source даёт явный отказ, refs/IDs не теряются |
| Нынешняя крыса как `.sdsheet` | 8 кадров row*2+column из 64×192 PNG, region32×48, Center(16,43), CenterFromMiddle=false, PPU60; alpha без потери светлого меха | Сохраняются CurrentFrame logic, PointClamp, upright scale, depth и feet; реальные кадры шага/направлений/паузы/перед стеной. Не вводится другая FSM или будущий трёхперсонажный арт |
| Собственный короткий WAV | Импорт `.sdsnd`, ссылка и громкость preview, повторный import изменённого сигнала | ReadyToPlay/Playing→Stopped плюс запись реального output loopback и обнаружение собственного временно-частотного сигнала против idle/volume-zero контроля. Exit0/исходный WAV не считаются слышимостью; отсутствие доступного output — явный незакрытый gate |
| Native UIPage + Noto/OFL | Кириллический TextBlock/layout/font reference, изменить текст/свойство, Save/reopen | Настоящий UIComponent в обычной игре и ZIP, читаемая кириллица720/1080, native font closure без установленного системного Noto; повреждённый font/ref отвергается |
| Sources/notices | Относительные source refs, creator/license/export inventory | ZIP содержит credits/notices и compiled data, не требует source/SDK/editor. Полный world/resource identity export сопоставляет normal/QA build |

Короткий sound/UI preview вызывается явной диагностической командой/контролом, без
постоянного шума при каждой загрузке сцены и без новой игровой системы. Звук проверяется
по выходу устройства; фактическое ручное прослушивание отдельно от automated loopback.
Изображения используются только для визуальной приёмки, не для управления MCP.

## Последовательность и финальные gates

1. Квалифицировать минимальные новые MCP операции на собственных временных assets,
   включая native Undo/dirty/revision, deterministic target, refs и async boundaries.
   Это первый самостоятельный code/review slice до переноса всех runtime ресурсов.
2. Создать resource definitions/sources и два настоящих prefab instances; проверить
   native import/reimport/override, затем отдельное владение resources в Windows.
3. Перевести rat sheet/font и добавить UI/sound preview; переключить normal game
   на те же authored references. Удалять active raw load лишь после compile/load gates.
4. Адаптировать прежние четыре raw PNG/font failures к native source/compiler/load
   equivalents с явным mapping. Не сохранять неиспользуемую копию ради зелёного теста.
   Сохранить остальные meaningful Core/authoring/executable regressions, добавить
   resource failure/candidate retirement и реальные визуальные/звуковые проверки.
5. Из verified commit собрать обычный и QA ZIP, запустить извлечённые пакеты из другого
   cwd, проверить closure/parity. Записать точные counts, hashes, editor/API calls,
   captures/audio evidence и ограничения; независимое review и полный Release editor
   выполняются до принятия A1.3. Ручная P1 и весь A1.4 этим не закрываются.

## Suggested Review Order

1. [A1 plan](stride-editor-asset-workflow-plan.md), границы/API и matrix выше.
2. [Нынешний MCP](../tools/stride-mcp/README.md), [adapter](../tools/stride-mcp/Rat.StrideMcp.Adapter/EditorBridge.cs).
3. [Core-only loader](../games/rat-expedition/Rat.Expedition.Authoring/NativeProjectLoader.cs),
   [scene lifetime](../games/rat-expedition/Rat.Expedition.Windows/ScenePresentation.cs).
4. Будущий implementation audit должен отличать source-inspected API от реально
   выполненных import/reimport/GUI/runtime и не объявлять все форматы редактора проверенными.
