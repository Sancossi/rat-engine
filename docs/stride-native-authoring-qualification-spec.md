# A1.1 — native проект и редактируемые metadata

Исполняемый срез [плана A1](stride-editor-asset-workflow-plan.md). Очередь и статусы остаются в [карточке](../vault/production/tasks/feat-stride-game-studio-authoring.md).

## Контракт среза

`games/rat-expedition/Rat.Expedition.Authoring.sln` открывается пересобранным Game Studio `4.4.0-dev` из закреплённого checkout. Библиотека `Rat.Expedition.Authoring` содержит native `.sdpkg`, сцену, процедурную модель, материал, compositor/GameSettings и сериализуемый `ExpeditionIdentityComponent`. Windows launcher загружает `QualificationScene` обычным `Content.Load<Scene>`; проверяет, что это тот же экземпляр, который загрузил SceneSystem из GameSettings.

Минимальная собственная сцена — зелёная коробка, камера и ambient light. `GameId` и `DisplayLabel` редактируются в property inspector. Сохранённое в GUI значение должно пережить Undo/Redo, закрытие и повторное открытие редактора, затем попасть в обычный F5 запуск и Release сборку wrapper. `loaded-asset.json` — выходное доказательство загрузки, не второй авторский источник.

P1 launcher, JSON мира, Core/FSM, камера и партия не мигрируют в этом срезе. Метаданные пока квалифицируют сериализацию, а не collision adapter. Уникальность `GameId` при prefab clone, трансформы, полный каталог ресурсов, rename/delete references и замена двух игровых сцен проверяются в A1.2–A1.4. Копирование компонента пока может скопировать строковый GameId; автоматическая уникальность не заявляется.

## Воспроизведение

1. `powershell -ExecutionPolicy Bypass -File scripts/stride/build-authoring.ps1` проверяет exact upstream, локальный feed и locked restore, публикует квалификационный launcher и запускает 60 кадров с другим cwd. Вывод — `build/stride-authoring/<timestamp>/result.json`, `runtime/loaded-asset.json`, `runtime/native-scene.png`. Это отдельный проверочный publish, не релизный ZIP P1.
2. Открыть `games/rat-expedition/Rat.Expedition.Authoring.sln` через pinned GameStudio. Открыть `QualificationScene`, выбрать `Qualification box`, раскрыть `Expedition identity`.
3. Изменить `DisplayLabel`, Enter, Save; Undo/Redo и повторное сохранение. Закрыть только свой редактор, открыть solution снова, проверить значение. Вызвать штатную команду Build the project and start the game (F5).
4. Обычный запуск без аргументов не завершает игру автоматически. Доказательства пишет в `%LOCALAPPDATA%/RatExpedition/authoring-qualification/`. Сравнить EntityId, GameId, DisplayLabel и transform с wrapper; затем штатно закрыть собственное окно игры.

Для повторяемого ввода доступен узкий [GUI helper](../scripts/stride/set-authoring-label.ps1): принимает PID своего GameStudio, ожидаемую старую и новую ASCII метку. Требует заранее выбранную сущность и раскрытое поле, находит строку UI Automation, проверяет PID под указателем и foreground перед вводом. Save/Undo/Redo остаются явными командами редактора. Это средство квалификации одного поля, не общий интерфейс автоматизации GameStudio.

## Подтверждённые точки pinned source

Все пути ниже относительно `C:/5_gamedev/stride` на `e2c786a45f69917bf233793f6a097b150e2fe264`:

- `samples/NewGame/NewGame/MyTemplate.Game/` и `MyTemplate.Windows/`: native package/asset версии, Engine + build-only AssetCompiler, `StrideCurrentPackagePath` и связь Windows → библиотека.
- `sources/engine/Stride.Engine/Engine/EntityComponent.cs`: сериализуемый компонент; `[DataContract]` / `[DataMember]` собственного типа дают native editor properties и runtime serializer.
- `sources/engine/Stride.Engine/Engine/SceneSystem.cs:106`: начальная сцена и compositor из GameSettings; `Game.cs:466` — асинхронная LoadContent.
- `sources/assets/Stride.AssetCompiler/build/Stride.AssetCompiler.targets`: сборка native asset database и перенос data в publish.

Compositor произведён от `MyTemplate.Game/Assets/GraphicsCompositor.LDR.sdgfxcomp` (Stride MIT): удалены archetype и неиспользуемые UI/Particles features, не входящие в минимальные зависимости. Сцена, модель, материал и metadata — собственные fixtures. Они не импортируют внешнюю графику. Ошибка отсутствующего serializer в первом варианте compositor исправлена в собственном asset; upstream не менялся.

## Suggested Review Order

1. [Сериализуемый компонент](../games/rat-expedition/Rat.Expedition.Authoring/ExpeditionIdentityComponent.cs) и [сохранённая сцена](../games/rat-expedition/Rat.Expedition.Authoring/Assets/QualificationScene.sdscene).
2. [Native package](../games/rat-expedition/Rat.Expedition.Authoring/Rat.Expedition.Authoring.sdpkg), [Windows project](../games/rat-expedition/Rat.Expedition.Authoring.Windows/Rat.Expedition.Authoring.Windows.csproj) и [загрузчик](../games/rat-expedition/Rat.Expedition.Authoring.Windows/Program.cs).
3. [Build wrapper](../scripts/stride/build-authoring.ps1), [GUI helper](../scripts/stride/set-authoring-label.ps1), [фактические доказательства](audits/2026-09-09-stride-native-authoring-qualification.md).
