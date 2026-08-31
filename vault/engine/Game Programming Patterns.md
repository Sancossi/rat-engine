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
- **Audio** — `QueuedAudio` + `AudioSink` (null/log) в `rat_core`; composition root `EditorApp` drain раз в кадр. Бэкенд — [[research: Audio backend ADR]]. Заготовка [[Audio Direction]].
- **Владение сервисами** — composition root `EditorApp`, без singleton/locator. Совпадает с [[Engine Vision]] («минимум скрытого глобального состояния»).
- **State** — `AppMode { Play, Edit }`, физические флаги `JumpState`, interpreter wait/message. Персонажного FSM нет.

## Глава → rat-engine

| GPP | У нас | Вердикт |
| --- | --- | --- |
| Game Loop | fixed 120 Hz в EditorApp | Оставить |
| Update Method | центральный tick | Оставить |
| Bytecode | EventRuntime opcodes | Уже правильный слой; follow-up [[feat: PlaySE event command]] |
| Command (ввод) | сырой GLFW | [[feat: Input action mapping]] → [[feat: Input rebind and gamepad]] |
| Command (редактор) | нет undo | [[feat: Edit undo/redo command stack]] |
| Event Queue (звук) | `QueuedAudio` FIFO, log/null sink | [[feat: PlaySE event command]] → [[research: Audio backend ADR]] |
| Service Locator | composition | Не заводить locator |
| Singleton | нет | Так и держать |
| State (FSM) | нет персонажного FSM | [[feat: Player locomotion FSM]] (Sprint 4); физика прыжка остаётся `JumpState` |
| Component / ECS | open question | [[research: Entity model ECS vs scene vs hybrid]] — ADR до любого ECS-кода |
| Observer | `GameplayNotifyBus` subscribe/post | stub; side channel (ItemPicked / DialogShown / Landed), not opcode interpreter. [[feat: Gameplay notify observer]] |
| Object Pool | нет | [[feat: Object pool for short-lived FX]] с field-action FX |
| Spatial Partition | height grid + linear blockers | [[feat: Spatial partition broadphase]] после профиля |
| Data Locality, Dirty Flag | нет | Не заводим без замера |
| Double Buffer | bgfx present | Достаточно |

GPP Command для ввода = кнопка → действие, ребинд, тот же интерфейс для AI. Сначала **кадр действий** (`InputFrame`), не объекты с undo.

GPP Event Queue для звука = геймплей постит `PlaySfx`, аудио забирает из очереди. Интерфейс `Audio*` прокидывать из `EditorApp`, не через глобальный locator.

## Очередь доработок

Порядок — зависимости, не даты. [[Sprint 4 — Refactoring and AI workflow]] ест Agent Debug и ближний GPP-слой движка; остальное — бэклог до триггера.

1. Ближний слой (Sprint 4): [[feat: Input action mapping]], [[feat: Audio play-queue stub]].
2. Сразу за звуком (Sprint 4): [[feat: PlaySE event command]], [[feat: Gameplay notify observer]].
3. Command в Edit (Sprint 4): [[feat: Edit undo/redo command stack]].
4. State (Sprint 4): [[feat: Player locomotion FSM]] — каркас имён до клипов; физика прыжка не в FSM.
5. Debug для агента (Sprint 4): [[Logging and assert helpers]], [[feat: Debug snapshot JSON]], [[feat: Event why-not-fired]], [[feat: Headless input sequence probe]] — [[Agent Debug]]. Тулзы: [[research: Rat debug loopback MCP]].
6. Authoring (бэклог): [[feat: Mouse viewport map edit]].
7. Следом за вводом/звуком (бэклог): [[feat: Input rebind and gamepad]], [[research: Audio backend ADR]].
8. По потребности (триггер в карточке): [[research: Entity model ECS vs scene vs hybrid]], [[feat: Object pool for short-lived FX]], [[feat: Spatial partition broadphase]].

## Не делаем

Глобальный Service Locator, Singleton, Data Locality / Dirty Flag без профиля, ECS в коде до ADR.
