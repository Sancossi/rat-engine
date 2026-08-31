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
- **Audio** — нет в коде. Заготовка [[Audio Direction]]. Задача [[feat: Audio play-queue stub]].
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
| Event Queue (звук) | нет аудио | [[feat: Audio play-queue stub]] → [[research: Audio backend ADR]] |
| Service Locator | composition | Не заводить locator |
| Singleton | нет | Так и держать |
| State (FSM) | нет персонажного FSM | [[feat: Player locomotion FSM]] после анимации / [[ADR-009 RE-like segmented character hierarchy]] |
| Component / ECS | open question | [[research: Entity model ECS vs scene vs hybrid]] — ADR до любого ECS-кода |
| Observer | нет шины | [[feat: Gameplay notify observer]] когда UI/аудио иначе лезут вглубь |
| Object Pool | нет | [[feat: Object pool for short-lived FX]] с field-action FX |
| Spatial Partition | height grid + linear blockers | [[feat: Spatial partition broadphase]] после профиля |
| Data Locality, Dirty Flag | нет | Не заводим без замера |
| Double Buffer | bgfx present | Достаточно |

GPP Command для ввода = кнопка → действие, ребинд, тот же интерфейс для AI. Сначала **кадр действий** (`InputFrame`), не объекты с undo.

GPP Event Queue для звука = геймплей постит `PlaySfx`, аудио забирает из очереди. Интерфейс `Audio*` прокидывать из `EditorApp`, не через глобальный locator.

## Очередь доработок (бэклог, без спринта)

Порядок — зависимости, не даты. Sprint 3 (высота/прыжок) эти карточки не ест.

1. Ближний слой: [[feat: Input action mapping]], [[feat: Audio play-queue stub]].
2. Authoring: [[feat: Mouse viewport map edit]] — Edit во вьюпорте мышью (сейчас только ImGui-списки).
3. Debug для агента (текст, не скрин): [[Logging and assert helpers]], [[feat: Debug snapshot JSON]], [[feat: Event why-not-fired]], [[feat: Headless input sequence probe]] — [[Agent Debug]]. Позже тулзы: [[research: Rat debug loopback MCP]].
4. Следом за вводом/звуком: [[feat: Input rebind and gamepad]], [[research: Audio backend ADR]], [[feat: PlaySE event command]].
5. По потребности (триггер в карточке): [[research: Entity model ECS vs scene vs hybrid]], [[feat: Player locomotion FSM]], [[feat: Edit undo/redo command stack]], [[feat: Gameplay notify observer]], [[feat: Object pool for short-lived FX]], [[feat: Spatial partition broadphase]].

## Не делаем

Глобальный Service Locator, Singleton, Data Locality / Dirty Flag без профиля, ECS в коде до ADR.
