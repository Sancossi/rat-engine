---
type: note
tags: [engine]
---

# Game Programming Patterns

Сверка с [Game Programming Patterns](https://www.gameprogrammingpatterns.com/) (Nystrom). Книга предупреждает не тащить паттерн «потому что глава есть» — ниже только то, что стыкуется с [[Architecture]] и текущим масштабом.

Аудит: 2026-08-31. Код: `apps/editor`, `src/engine` (`rat_core` / `rat_engine`).

## Как сейчас устроено

- **Game Loop** — «play catch up»: переменный `dt` + аккумулятор 120 Hz + clamp 0.1s / spiral cap 0.2s в `EditorApp::run` / `update_simulation`. Рендер отдельно после симуляции. Живёт в editor-shell, не в `rat_core`.
- **Update Method** — центральные `update_simulation`, `EventRuntime::update`, `integrate_player_frame_surface`. Нет иерархии виртуальных `update()` на сущностях.
- **Bytecode / Interpreter** — `Command` + `CommandOp` + стек interpreter в `EventRuntime`. Data-driven VM, не GoF Command. Совпадает с [[Event System]] и [[ADR-007 Events and maps stored as JSON]].
- **Input** — сырой `glfwGetKey` в `EditorApp`. Зачатки decoupling: `MoveInput` / `PlayerFrameInput` уже без GLFW. Action map, ребинд, геймпад — нет. Слой «raw → actions» в [[Architecture]]; задача [[feat-input-action-mapping|feat: Input action mapping]].
- **Edit Command** — `EditHistory` / `EditCommand` в `rat_core`, владелец `EditorApp`. Place/move/delete blockers и events; Play не пишет стек; `clear()` на hot-apply.
- **Audio** — `QueuedAudio` + `AudioSink` в `rat_core`; composition root `EditorApp` drain раз в кадр. Device — miniaudio как sink ([[ADR-013 Audio backend miniaudio]]); тесты подменяют Recording/Null. Клипы — `AssetId`. Заготовка [[Audio Direction]].
- **Владение сервисами** — composition root `EditorApp`, без singleton/locator. Совпадает с [[Engine Vision]] («минимум скрытого глобального состояния»).
- **State** — `AppMode { Play, Edit }`, физические флаги `JumpState`, interpreter wait/message. Персонажный FSM: `locomotion_from` → Idle/Walk/Jump/Fall/**Climb**; тик `integrate_player_frame_surface` выбирает Climb vs ground/air.

## Глава → rat-engine

| GPP | У нас | Вердикт |
| --- | --- | --- |
| Game Loop | fixed 120 Hz в EditorApp | Оставить |
| Update Method | центральный tick | Оставить |
| Bytecode | EventRuntime opcodes | Уже правильный слой; follow-up [[feat-play-se-event-command|feat: PlaySE event command]] |
| Command (ввод) | сырой GLFW | [[feat-input-action-mapping|feat: Input action mapping]] → [[feat-input-rebind-gamepad|feat: Input rebind and gamepad]] |
| Command (редактор) | `EditHistory` / `EditCommand` (place/move/delete blockers+events) | Готово [[feat-edit-undo-redo|feat: Edit undo/redo command stack]] |
| Event Queue (звук) | `QueuedAudio` FIFO + overflow_count; tests Recording/Null; editor miniaudio sink | Готово [[feat-audio-backend-implementation|feat: Audio backend implementation]] |
| Service Locator | composition | Не заводить locator |
| Singleton | нет | Так и держать |
| State (FSM) | `locomotion_from` Idle/Walk/Jump/Fall/Climb; Climb владеет tick на Interact-rail ([[feat-mgs3-ladder-climb|feat: MGS3 ladder climb]]) | Готово [[feat-drive-player-physics-from-locomotion-fsm|feat: Drive player physics from locomotion FSM]]; клипы — [[ADR-009 RE-like segmented character hierarchy]] |
| Component / ECS | гибрид EntityId + ComponentStore | [[ADR-012 Entity model EntityId and ComponentStore]]; игрок/события пока POD. Не archetype |
| Observer | `GameplayNotifyBus` subscribe/post | stub; side channel (ItemPicked / DialogShown / Landed), not opcode interpreter. [[feat-gameplay-notify-observer|feat: Gameplay notify observer]] |
| Object Pool | нет | [[feat-object-pool-fx|feat: Object pool for short-lived FX]] с field-action FX |
| Spatial Partition | height grid + linear blockers | [[feat-spatial-partition-broadphase|feat: Spatial partition broadphase]] после профиля |
| Data Locality, Dirty Flag | нет | Не заводим без замера |
| Double Buffer | bgfx present | Достаточно |

GPP Command для ввода = кнопка → действие, ребинд, тот же интерфейс для AI. Сначала **кадр действий** (`InputFrame`), не объекты с undo.

GPP Event Queue для звука = геймплей постит `PlaySfx`, аудио забирает из очереди. Интерфейс `Audio*` и конкретный `AudioSink` прокидывать из `EditorApp`, не через глобальный locator. Device-sink — miniaudio ([[ADR-013 Audio backend miniaudio]]); тесты оставляют `RecordingAudioSink` / `NullAudioSink`.

## Очередь доработок

Порядок — зависимости, не даты. Sprint 4 закрыл Agent Debug и ближний GPP-слой; архитектурная очередь закрыта в [[Sprint 7 — Engine architecture]].

1. Ближний слой (Sprint 4): [[feat-input-action-mapping|feat: Input action mapping]], [[feat-audio-play-queue-stub|feat: Audio play-queue stub]].
2. Сразу за звуком (Sprint 4): [[feat-play-se-event-command|feat: PlaySE event command]], [[feat-gameplay-notify-observer|feat: Gameplay notify observer]].
3. Command в Edit (Sprint 4): [[feat-edit-undo-redo|feat: Edit undo/redo command stack]].
4. State (Sprint 4): [[feat-player-locomotion-fsm|feat: Player locomotion FSM]] — готово (`locomotion_from`); клипы не в этой карточке.
5. Debug для агента (Sprint 4): [[logging-and-assert-helpers|Logging and assert helpers]], [[feat-debug-snapshot-json|feat: Debug snapshot JSON]], [[feat-event-why-not-fired|feat: Event why-not-fired]], [[feat-headless-input-sequence-probe|feat: Headless input sequence probe]] — [[Agent Debug]]. Тулзы: [[research-rat-debug-loopback-mcp|research: Rat debug loopback MCP]].
6. Authoring (Sprint 6, Done): [[feat-mouse-viewport-map-edit|feat: Mouse viewport map edit]] → [[feat-undo-height-and-edge-edits|feat: Undo height-grid and edge edits]] → [[feat-mouse-viewport-terrain-edit|feat: Mouse viewport terrain edit]].
7. Архитектура (Sprint 7): [[feat-simulation-session-unified-tick|feat: SimulationSession unified tick]] → [[feat-replay-recording-and-checksum|feat: Replay recording and checksum]] → [[feat-map-document-and-runtime-map|feat: MapDocument and RuntimeMap compile]] → [[chore-decompose-editor-app|chore: Decompose EditorApp]]; ввод/звук: [[feat-input-rebind-gamepad|feat: Input rebind and gamepad]], [[ADR-013 Audio backend miniaudio]] → [[feat-audio-backend-implementation|feat: Audio backend implementation]]; entity ADR: [[research-entity-model-ecs-vs-scene|research: Entity model ECS vs scene vs hybrid]].
8. По потребности после метрик: [[feat-object-pool-fx|feat: Object pool for short-lived FX]], [[feat-spatial-partition-broadphase|feat: Spatial partition broadphase]].

## Не делаем

Глобальный Service Locator, Singleton, Data Locality / Dirty Flag без профиля, archetype ECS / job system.
