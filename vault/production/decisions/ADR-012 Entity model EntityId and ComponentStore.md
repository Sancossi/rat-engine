---
type: adr
area: Engine
status: Accepted
decided: 2026-09-01
tags: [adr]
---

# ADR-012 Entity model: EntityId and ComponentStore

## Context

Карта MVP — POD-события, один `PlayerBody` и blockers. Несколько динамических тел (герой, ящики, двери) могут жить как массив `CollisionBody` без ECS. Open question в [[Architecture]] (GPP Component): ECS vs scene-graph vs гибрид.

Триггер переписывания — **не** коллизии и не [[feat-field-physics-puzzles|feat: Field physics puzzles]]. Имеет смысл менять модель, когда разнородных акторов с разными наборами компонент станет много, или когда сцена-граф начнёт дублировать Transform/Renderable/Collider в трёх местах.

Сравнение (этап 6 [architecture roadmap](../../../docs/archive/cpp/architecture-roadmap.md)):

| Вариант | Сильные стороны | Цена сейчас |
| --- | --- | --- |
| Текущие POD + managers | Уже работает; один игрок, события, `SimulationSession` без лишнего слоя | Нет общего identity; RenderWorld/физика/сцена рискуют копировать одни и те же поля |
| Scene-graph как identity | Parent/child transforms; удобно для костей | Для RM-like карты иерархия не нужна как модель сущностей. Визуальная иерархия персонажа уже [[ADR-009 RE-like segmented character hierarchy]] |
| Archetype ECS + jobs | Плотные архетипы, запросы, параллельные системы | Переписать игрока, события и tick; нет измеримой нагрузки и разнородности |
| Гибрид `EntityId` + `ComponentStore` | Стабильный id с generation; sparse-слоты Transform/Renderable/Collider; явный `for_each` | Маленький API в `rat_core`; симуляция MVP остаётся POD |

## Decision

**Гибридный минимум:** `EntityId` (index + generation) и `ComponentStore<T>` для `Transform` / `Renderable` / `Collider` в `rat_core`. Итерация явная, без archetype, query compiler и job system.

`PlayerBody`, event runtime и `SimulationSession` **не** мигрируют на stores в этом решении. Следующий потребитель — [[feat-render-world-packets|feat: RenderWorld packets and instrumentation]].

Переписывать / расширять:

- Добавить новый `ComponentStore`, когда **два и более** вида акторов делят одно поле (например Velocity или Interactable) и копирование POD уже расходится.
- Подключать игрока или события к `EntityId` только отдельной задачей, когда RenderWorld или физика реально читают те же stores.
- Archetype ECS — только если разнородных акторов много **и** профиль показывает, что sparse-итерация/`unordered` lookup — bottleneck (cache misses, не «хочется ECS»).
- Scene-graph — только для визуального parent/child (персонаж ADR-009, props), не как identity мира.

## Consequences

- Open question Entity model в [[Architecture]] закрыт.
- Lifecycle и stale-id покрыты тестами (`create` / `destroy` / generation bump / `ComponentStore` отвергает мёртвый id).
- Новые динамические акторы могут получить id без смены tick-контракта.
- Полноценный ECS, RHI и job system остаются вне scope до замера.
