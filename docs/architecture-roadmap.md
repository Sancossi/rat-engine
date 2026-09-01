# Архитектурный roadmap rat-engine

## Контекст

Roadmap основан на реализации из `origin/feat/sprint-4`. В `main` на момент
составления документа находится только стартовый README, поэтому первым
условием архитектурных работ остаётся интеграция актуального кода движка в
основную ветку.

Цель roadmap — последовательно отделить симуляцию, authoring, представление и
платформенные адаптеры, не внедряя ECS, RHI или job system до появления
измеримой необходимости.

## Архитектурные принципы

- Gameplay не зависит от GLFW, ImGui, bgfx и API операционной системы.
- Симуляция выполняется единственным фиксированным tick независимо от способа
  запуска.
- Редактируемый документ карты отделён от неизменяемых runtime-данных.
- Render, audio и editor получают данные через явные интерфейсы и очереди, без
  singleton и Service Locator.
- Оптимизации вводятся только после профилирования и закрепляются benchmark.
- Крупные этапы выполняются отдельными небольшими PR.

## Целевая структура

```text
apps/editor
  GLFW/ImGui, панели и composition root

rat_authoring
  MapDocument, validation, migrations и undo/redo

rat_runtime
  SimulationSession, RuntimeWorld, Event VM и replay

rat_render
  RenderWorld, render packets и bgfx backend

rat_assets
  Asset registry, loading и compiled resources

rat_core
  IDs, math, logging, время и общие типы
```

## Этап 0. Рабочая база и CI

### Работы

- Интегрировать актуальную ветку движка в `main`.
- Добавить CI-сборку `rat_core`, editor и Catch2 для Windows и Linux.
- Добавить headless smoke-сценарий карты `grey_yard`.
- Зафиксировать граф зависимостей библиотек.

### Критерии готовности

- Чистый checkout собирается без локальных ручных шагов.
- Все headless-тесты проходят.
- `rat_core` не линкуется с GLFW, ImGui или bgfx.

## Этап 1. Единое ядро симуляции

Сейчас fixed-step логика распределена между
`apps/editor/editor_app.cpp` и `src/engine/src/input_sequence.cpp`. Это создаёт
риск расхождения интерактивного и headless-поведения.

### Работы

- Добавить `SimulationSession`, владеющий `PlayerBody`, `JumpState`,
  `GameState`, `EventRuntime` и монотонным `tick_id`.
- Ввести единый контракт:

  ```cpp
  SimulationTickResult tick(const InputFrame& input);
  ```

- Перенести в tick движение игрока, события, transfer player, landing
  notifications и interaction buffering.
- Оставить accumulator и presentation interpolation за пределами симуляции.
- Превратить `run_input_sequence()` в адаптер над `SimulationSession`.
- Ограничить число catch-up ticks и диагностировать превышение бюджета.

### Критерии готовности

- Editor и headless runner используют одну реализацию tick.
- Результат симуляции не зависит от display FPS.
- Существующие тесты механик сохраняют поведение.
- Simulation API не принимает platform- или render-типы.

## Этап 2. Replay и воспроизводимость

### Работы

- Добавить сериализуемые `ReplayHeader`, `TickInput` и `ReplayRecording`.
- Записывать `InputFrame` вместе с `tick_id`.
- Ввести явный seed для будущих генераторов случайных чисел.
- Рассчитывать checksum существенного runtime-состояния.
- Расширить debug snapshot данными tick, input и checksum.
- При расхождении replay сообщать первый несовпавший tick.

### Критерии готовности

- Повторный запуск одной записи даёт идентичный checksum.
- Одна запись одинаково воспроизводится через editor и headless API.
- Replay-тест покрывает загрузку карты, движение и выполнение event-команды.

## Этап 3. Граница authoring/runtime

`EventRuntime` сейчас одновременно интерпретирует события, хранит `MapData` и
предоставляет операции изменения terrain. Эти обязанности необходимо
разделить.

### Работы

- Ввести `MapDocument` как владельца terrain, ramps, blockers, edge barriers и
  event definitions.
- Удалить authoring-операции высот и ramps из `EventRuntime`.
- Ввести pipeline:

  ```text
  JSON
    -> schema validation
    -> migration
    -> semantic validation
    -> MapDocument
    -> compile
    -> RuntimeMap
  ```

- Сделать `RuntimeMap` неизменяемым в течение simulation tick.
- Добавить ревизии документа и явную инвалидацию производных данных.
- Возвращать структурированные ошибки с severity, JSON path и сообщением.
- Проверять уникальность ID, ссылки, размеры grid, ramps, barriers и команды.
- Расширить `EditHistory` на height grid, ramps и edge barriers.

### Критерии готовности

- `EventRuntime` не изменяет карту.
- Некорректная карта отклоняется до запуска симуляции с точным путём ошибки.
- Undo/redo восстанавливает семантически идентичный `MapDocument`.
- Save/load round-trip сохраняет смысл документа.

## Этап 4. Декомпозиция EditorApp

### Целевая структура

```text
apps/editor/
  editor_app
  editor_document
  editor_session
  frame_coordinator
  panels/
    blocker_panel
    terrain_panel
    event_panel
  platform/
    glfw_host
```

### Ответственность

- `EditorApp` создаёт и связывает зависимости.
- `GlfwHost` владеет окном и сырым вводом.
- `FrameCoordinator` задаёт порядок фаз кадра.
- `EditorDocument` владеет картой, selection, dirty state и history.
- Панели формируют команды документа, но не меняют runtime напрямую.
- `SimulationSession` владеет gameplay-состоянием.

### Критерии готовности

- В `EditorApp` отсутствует реализация игровой физики.
- Панели не вызывают mutation-методы `EventRuntime`.
- Play/Edit переключение не уничтожает authoring state.
- Операции документа тестируются без окна и ImGui.

## Этап 5. Минимальный asset pipeline

### Работы

- Добавить `AssetId`, `AssetPath`, `AssetState`, `AssetRegistry` и
  `AssetLoader`.
- Разделить source, compiled, CPU и GPU representations.
- Реализовать состояния `Unloaded`, `Loading`, `Ready`, `Failed`.
- Начать с textures, audio clips и mesh/material descriptors.
- Добавить fallback assets, debug names и диагностические ошибки.
- Реализовать editor hot reload без зависимости gameplay от файловой системы.
- Выполнять уничтожение GPU-ресурсов после безопасной границы кадра.

### Критерии готовности

- Карты и gameplay ссылаются на стабильные `AssetId`.
- Отсутствующий или повреждённый asset не приводит к аварийному завершению.
- Headless-тесты подменяют registry без GPU и файловой системы.

## Этап 6. Решение по entity model

До реализации необходимо принять ADR, сравнивающий текущие POD-структуры,
manager-driven components, archetype ECS и гибридную модель.

Рекомендуемый минимальный вариант:

```text
EntityId + generation
ComponentStore<Transform>
ComponentStore<Renderable>
ComponentStore<Collider>
Явные системные проходы
```

Archetype migration, query compiler, generic scheduler и job system остаются
вне scope до измеримого запроса.

### Критерии готовности

- Принят ADR с выбранной моделью.
- ADR содержит критерии будущего расширения или замены.
- Lifecycle сущностей и защита от устаревших ID покрыты тестами.

## Этап 7. Render architecture

### Поток данных

```text
SimulationSession
  -> RenderWorld
  -> RenderPackets
  -> RenderPasses
  -> bgfx backend
```

### Работы

- Ввести render-only snapshot с интерполированными transforms.
- Добавить сортировку draw packets и material handles.
- Добавить frame allocator и отложенное освобождение ресурсов.
- Интегрировать CPU/GPU markers, debug names и RenderDoc capture hook.
- Сохранить bgfx как backend abstraction; отдельный универсальный RHI не
  создавать.

### Критерии готовности

- Renderer не принимает `GameState`, `EventRuntime` или editor-типы.
- Render snapshot можно построить без непосредственного вызова bgfx.
- Основные passes и расходы кадра видны в profiler/frame capture.

## Этап 8. Platform, input и audio

### Input

- Табличный action mapping.
- Keyboard/gamepad adapters и rebind.
- Сериализация bindings.
- Через границу simulation проходит только `InputFrame`.

### Platform

- Скрыть Win32-типы за минимальным `NativeWindow`.
- Поддержать подходящее представление окна для Windows и Linux.
- Инкапсулировать часы ОС и файловые операции узкими интерфейсами.

### Audio

- Принять ADR по backend.
- Сохранить `QueuedAudio` и заменить log/null sink реальной реализацией.
- Использовать asset IDs для clips.
- Ограничить очередь и публиковать метрики переполнения.

## Этап 9. Производительность по измерениям

До оптимизации добавить метрики:

- длительность simulation tick;
- event commands per tick;
- draw calls;
- transient allocations;
- asset uploads;
- audio queue depth;
- collision candidates.

Spatial partition, object pools, SoA, dirty flags, frequency tiers и job system
добавляются только при подтверждённом bottleneck. Каждая такая работа должна
содержать benchmark до и после изменения.

## Порядок PR

Текущий Sprint 5 с terrain cubes и edge walls не следует смешивать с
архитектурным рефакторингом. Рекомендуемый порядок независимых PR:

1. Terrain cubes.
2. Edge barriers.
3. Editor/greybox edge walls.
4. `SimulationSession`.
5. Replay и state checksum.
6. `MapDocument` и compilation в `RuntimeMap`.
7. Terrain/barrier undo.
8. Декомпозиция `EditorApp`.
9. Asset registry и первая загрузочная вертикаль.
10. Entity model ADR.
11. Render packets и instrumentation.
12. Platform/input/audio adapters.

## Явно вне ближайшего scope

- Глобальный Service Locator.
- Singleton-подсистемы.
- Полноценный archetype ECS.
- Собственный универсальный RHI.
- Job system.
- Networking и rollback.
- DOD/SoA без результатов профилирования.

## Источники решений

- [Game Programming Patterns](https://gameprogrammingpatterns.com/)
- [Game Engine Architecture](https://www.gameenginebook.com/)
- [Data-Oriented Design](https://dataorienteddesign.com/dodbook/node2.html)
- [Fix Your Timestep](https://gafferongames.com/post/fix_your_timestep/)
- [The Mature Optimization Handbook](https://carlos.bueno.org/optimization/)
- [RenderDoc](https://renderdoc.org/)

