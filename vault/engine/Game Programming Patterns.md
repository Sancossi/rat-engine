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
- **Input** — сырой `glfwGetKey` в `EditorApp`. Зачатки decoupling: `MoveInput` / `PlayerFrameInput` уже без GLFW. Action map, ребинд, геймпад — нет. Слой «raw → actions» в [[Architecture]]; задача [[feat: Input action mapping]].
- **Edit Command** — `EditHistory` / `EditCommand` в `rat_core`, владелец `EditorApp`. Place/move/delete blockers и events; Play не пишет стек; `clear()` на hot-apply.
- **Audio** — `QueuedAudio` + `AudioSink` в `rat_core`; composition root `EditorApp` drain раз в кадр. Device — miniaudio как sink ([[ADR-013 Audio backend miniaudio]]); тесты подменяют Recording/Null. Клипы — `AssetId`. Заготовка [[Audio Direction]].
- **Владение сервисами** — composition root `EditorApp`, без singleton/locator. Совпадает с [[Engine Vision]] («минимум скрытого глобального состояния»).
- **State** — `AppMode { Play, Edit }`, физические флаги `JumpState`, interpreter wait/message. Персонажный classify: `locomotion_from` → Idle/Walk/Jump/Fall (`rat/locomotion.hpp`); физика прыжка не в FSM.

## Глава → rat-engine

| GPP | У нас | Вердикт |
| --- | --- | --- |
| Game Loop | fixed 120 Hz в EditorApp | Оставить |
| Update Method | центральный tick | Оставить |
| Bytecode | EventRuntime opcodes | Уже правильный слой; follow-up [[feat: PlaySE event command]] |
| Command (ввод) | сырой GLFW | [[feat: Input action mapping]] → [[feat: Input rebind and gamepad]] |
| Command (редактор) | `EditHistory` / `EditCommand` (place/move/delete blockers+events) | Готово [[feat: Edit undo/redo command stack]] |
| Event Queue (звук) | `QueuedAudio` FIFO + overflow_count; tests Recording/Null; editor miniaudio sink | Готово [[feat: Audio backend implementation]] |
| Service Locator | composition | Не заводить locator |
| Singleton | нет | Так и держать |
| State (FSM) | `locomotion_from` Idle/Walk/Jump/Fall; физика в `JumpState` | Готово [[feat: Player locomotion FSM]]; клипы — позже по [[ADR-009 RE-like segmented character hierarchy]] |
| Component / ECS | гибрид EntityId + ComponentStore | [[ADR-012 Entity model EntityId and ComponentStore]]; игрок/события пока POD. Не archetype |
| Observer | `GameplayNotifyBus` subscribe/post | stub; side channel (ItemPicked / DialogShown / Landed), not opcode interpreter. [[feat: Gameplay notify observer]] |
| Object Pool | нет | [[feat: Object pool for short-lived FX]] с field-action FX |
| Spatial Partition | height grid + linear blockers | [[feat: Spatial partition broadphase]] после профиля |
| Data Locality, Dirty Flag | нет | Не заводим без замера |
| Double Buffer | bgfx present | Достаточно |

GPP Command для ввода = кнопка → действие, ребинд, тот же интерфейс для AI. Сначала **кадр действий** (`InputFrame`), не объекты с undo.

GPP Event Queue для звука = геймплей постит `PlaySfx`, аудио забирает из очереди. Интерфейс `Audio*` и конкретный `AudioSink` прокидывать из `EditorApp`, не через глобальный locator. Device-sink — miniaudio ([[ADR-013 Audio backend miniaudio]]); тесты оставляют `RecordingAudioSink` / `NullAudioSink`.

## Очередь доработок

Порядок — зависимости, не даты. Sprint 4 закрыл Agent Debug и ближний GPP-слой; архитектурная очередь закрыта в [[Sprint 7 — Engine architecture]].

1. Ближний слой (Sprint 4): [[feat: Input action mapping]], [[feat: Audio play-queue stub]].
2. Сразу за звуком (Sprint 4): [[feat: PlaySE event command]], [[feat: Gameplay notify observer]].
3. Command в Edit (Sprint 4): [[feat: Edit undo/redo command stack]].
4. State (Sprint 4): [[feat: Player locomotion FSM]] — готово (`locomotion_from`); клипы не в этой карточке.
5. Debug для агента (Sprint 4): [[Logging and assert helpers]], [[feat: Debug snapshot JSON]], [[feat: Event why-not-fired]], [[feat: Headless input sequence probe]] — [[Agent Debug]]. Тулзы: [[research: Rat debug loopback MCP]].
6. Authoring (Sprint 6, Done): [[feat: Mouse viewport map edit]] → [[feat: Undo height-grid and edge edits]] → [[feat: Mouse viewport terrain edit]].
7. Архитектура (Sprint 7): [[feat: SimulationSession unified tick]] → [[feat: Replay recording and checksum]] → [[feat: MapDocument and RuntimeMap compile]] → [[chore: Decompose EditorApp]]; ввод/звук: [[feat: Input rebind and gamepad]], [[ADR-013 Audio backend miniaudio]] → [[feat: Audio backend implementation]]; entity ADR: [[research: Entity model ECS vs scene vs hybrid]].
8. По потребности после метрик: [[feat: Object pool for short-lived FX]], [[feat: Spatial partition broadphase]].

## Не делаем

Глобальный Service Locator, Singleton, Data Locality / Dirty Flag без профиля, archetype ECS / job system.
