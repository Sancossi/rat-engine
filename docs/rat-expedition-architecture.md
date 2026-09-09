# Rat Expedition — архитектура Stride / C#

Дата: 2026-09-09. Действующий контракт по [ADR-018](../vault/production/decisions/ADR-018%20Rat%20expedition%20uses%20Stride.md), [GDD](../vault/game/GDD.md) и [P1](rat-expedition-traversal-spec.md).
[Архив C++ архитектуры](archive/cpp/rat-expedition-architecture-rat-engine-2026-09-08.md) сохраняет прежний план и происхождение решений. Ниже описаны текущие интерфейсы и оставшиеся системы; точный статус ведётся в vault, сборка Game Studio не доказывает готовность игры.

## Основа и размещение

[Engine lock](../tools/stride/engine.lock.json) закрепляет Stride upstream `e2c786a45f69917bf233793f6a097b150e2fe264`, проверенную сборку `4.4.0-dev`, .NET SDK `10.0.300`, Windows x64/Direct3D11. Это development snapshot 4.4, не stable. Движок остаётся в `C:/5_gamedev/stride`; [workflow](stride-source-workflow.md) описывает его сборку и собственные патчи.

Игра размещается в `games/rat-expedition/` вне upstream. Прежние C++ исходники, CMake и карты/replay/save удалены из рабочего дерева 2026-09-09; они доступны в Git по baseline из [архива](archive/cpp/README.md). Игра использует собственные ресурсы из `games/rat-expedition/Content/`.

| Путь под games/rat-expedition | Ответственность |
| --- | --- |
| Rat.Expedition.Core/ | Чистые C# правила, команды, снимки, валидация; без Stride/GPU |
| Rat.Expedition.Windows/ | net10.0-windows executable, Stride Game, ввод, сцена, камера, спрайты, диагностика |
| Rat.Expedition.Core.Tests/ | Наблюдаемые сценарии правил без окна |
| Content/ | Игровые JSON/PNG, credits.json, нужные fonts/notices |
| tools/ | Исходник собственного временного PNG |
| NuGet.Config, Directory.Build.props, проектные packages.lock.json | Общие настройки и закреплённые зависимости |

`scripts/stride/build-game.ps1` проверяет исходную основу, собирает/тестирует/публикует игру и создаёт ZIP в `build/stride-game/`. First-party `Stride.*` — точная `4.4.0-dev` из `stride/bin/packages`; source mapping направляет namespace в local feed. Более точное исключение `Stride.Dependencies.*` идёт в NuGet вместе с остальными внешними зависимостями и фиксируется lock-файлами. Не подменять отсутствующий first-party пакет stable/плавающей версией. Суффикс dev не идентифицирует SHA: manifest сборки записывает baseline, фактический HEAD, package versions и SDK. При необходимости использовать отдельный cache игры, не очищать глобальный.

## Первый срез и поток данных

[Исполнимый P1.1](stride-first-game-slice-spec.md) ввёл плоский двор, стены, PNG героя и WASD. [P1.2](stride-body-traversal-spec.md) расширяет его конечными потолками/площадкой и лестницей после отдельной квалификации body queries (`1a2e7cd`). Видимая solid геометрия и ограничения строятся из одной валидированной scene schema 2. Rungs/rails лестницы — декоративное обозначение контекстного маршрута, не solid blockers. Bepu и патчи Stride не потребовались; это не универсальная многоэтажная физика.

`Stride Input → TraversalInput → fixed-step Core → TraversalSnapshot → Entity/Transform/спрайт`.

Core владеет позицией; представление применяет снимок. Camera right/forward проецируются на XZ, диагональ нормируется. Шаг 1/120 с, ограниченный catch-up; после потери фокуса нет накопленного скачка. P1.1 блокирует движение в неактивном окне; полная пауза/контекст — P1.3.

P1.2 использует явный FSM игрока Standing/Crouched/Climbing. Одна state-переменная определяет stance/mode снимка; guarded переходы используют BodyCollisionWorld для полного объёма, нового взаимодействия и безопасного выхода. Внутри Climbing различаются подход, вертикальный участок и выход. Это локальная логика motor без универсального FSM framework; renderer не выбирает состояние. Квалифицированный adapter конечных 3D коробок проверяет explicit feet Y и полный footprint на одной опоре, swept clearance и путь лестницы; смена высоты разрешена только явным переходом.

Подтверждённый source API: `Game.LoadContent`, `Scene/SceneInstance`, `Entity`, `CameraComponent`, `GraphicsCompositorHelper.CreateDefault(false, camera: ...)`, `ModelComponent`, `SpriteComponent`. `CubeProceduralModel.Generate(Services)` создаёт коробки с bounds для culling. PNG/atlas использует `Texture.Load` и `SpriteFromTexture/SpriteFromSheet`; `SpriteType.Billboard`, `SpriteSampler.PointClamp`, `IgnoreDepth=false`. Обычный straight-alpha PNG требует согласования с `PremultipliedAlpha`: default Texture.Load сохраняет alpha, default SpriteComponent предполагает premultiplied. Материал, shader database и publish closure проверяются запуском; перенос bgfx/GLFW не требуется.

## Полный P1: пространство, состояние, представление

[Полный контракт](rat-expedition-traversal-spec.md) сохраняет лаз, лестницу, мост/рампу, два маршрута в одинаковых XZ, переходы, recovery, спутников и локальные перекрытия. Их C++ реализация остаётся историей.

Мир — конечные 3D объёмы настила, потолка, стен/опор/рампы. Опора учитывает высоту ног, движение и clearance, не выбирается одним height lookup по XZ или номером этажа. Приседание меняет принятый объём тела до motor step; картинка не заменяет коллизии. Контекст сначала фильтруется по высоте/преградам, затем nearest/id.

Полная `ExpeditionSession` владеет активной сценой/состоянием, безопасной точкой, историей 3D пути спутников и Explore/Paused. Загрузчик подготавливает scene candidate, проверяет данные/ссылки/spawn и лишь затем заменяет активную сцену. Ошибка сохраняет прежнюю сцену/позицию и освобождает ресурсы кандидата. Portal/recovery очищают pending input, accumulator, trail и occlusion history; все герои оказываются на проверенной опоре.

P1 authoring начинается с game JSON и code-first сцены. [План A1](stride-editor-asset-workflow-plan.md) предлагает после полного P1, перед P2, перенести карты и категории игровых ресурсов в native проект Game Studio. Scene/prefab, устойчивые игровые ids и свойства объёмов становятся одним авторским источником для представления и валидированных данных Core; ручной JSON-дубль не сохраняется. Пределы transforms явны: текущие axis-aligned solids допускают трансляцию/положительные размеры, но не произвольные rotation/scale или преобразования родителей; остальные объёмы полного P1 квалифицируются отдельно. Импорт/размещение/native свойства отличаются от создания mesh, пикселей и аудио во внешних инструментах. Точные editor/compiler API и parity обычного запуска с самостоятельным ZIP ещё предстоит проверить на locked source. Это основа до массового контента E3, не реализованный авторинг и не универсальный RPG editor; карточка A1 задаёт зависимости и приёмку.

Occlusion — presentation adapter с локальными группами model entities и прикреплённых деталей. Настоящие пересечения ортографических лучей до глубины героя определяют скрытие; bounds лишь broad phase. Коллизии/цели не меняются. Опорный настил лидера видим; его перекрывающие стены/перила при необходимости отдельны. Вырез верхнего участка скрывает связанный декор и изображение верхнего спутника локально, но не нижнего. Геометрические условия и исключения заданы полным P1.

## Рост после P1

P2 добавляет общий инвентарь/предметные правила, P3 квест/обещания/диалоги, P4 партию/бой. Данные принадлежат Core/Session; UI не держит вторую копию ресурса. Не создавать общий RPG framework до конкретных карточек.

P5 хранит всю экспедицию в `%LOCALAPPDATA%/RatExpedition/` с тестовым override. Полное чтение/валидация и подготовка сцены предшествуют единому применению; запись через временный файл и атомарную замену. Формат задаётся в P5, без автоматической совместимости со старым editor save/replay. P1 не создаёт persistent slot.

## Доставка и доказательства

Self-contained win-x64 publish запускается без editor, SDK, NuGet, сети и source checkout. ZIP содержит app/runtime/native libraries, нужные shaders/content, credits, проектную лицензию и notices фактически включённых зависимостей. Проверять извлечённый пакет из другого cwd. Данные читаются от `AppContext.BaseDirectory`; диагностические записи идут в явно заданный каталог.

Валидатор отвергает отсутствующие/повреждённые PNG/JSON, бесконечные числа, неверные bounds и escaping paths. Стартовый отказ — читаемая ошибка/ненулевой exit; переходный отказ полного P1 сохраняет текущую сцену.

Evidence разделяет Core tests, сборку, реальный GPU/adapter, автоматический запуск и ручную проверку. PNG 1280×720/1920×1080 снимаются с настоящего backbuffer; диаграмма и тест без GPU не заменяют кадр. A7 бюджеты не объявляются выполненными до измерений; историческая инвентаризация ПК сохранена в архиве архитектуры.

MCP/удалённые сервисы/изменение Stride этому срезу не нужны. Статусы и следующий разрешённый срез задаёт vault; старую C++ очередь автоматически не продолжать.
