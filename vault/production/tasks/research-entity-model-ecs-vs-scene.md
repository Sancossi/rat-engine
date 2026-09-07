---
type: task
area: Engine
status: Done
task_type: Research
sprint: Sprint 7
due:
tags: [task]
---

# research: Entity model ECS vs scene vs hybrid

Intent: закрыть open question в [[Architecture]] (GPP Component). Сейчас карта = POD-события + player + blockers, ECS не нужен. Спайк сравнивает ECS / scene-graph / hybrid «игрок+события как сейчас». Контекст: [[Game Programming Patterns]].

Триггер **не** коллизии и не [[feat-field-physics-puzzles|feat: Field physics puzzles]]: несколько динамических тел (герой, ящики, двери) живут как массив `CollisionBody` без ECS. Переписывать имеет смысл, когда разнородных акторов с разными наборами компонент станет много, или когда сцена-граф начнёт дублировать то же самое в трёх местах. До ADR в код ECS не класть.

Acceptance: ADR (следующий номер после ADR-011) с выбором и критерием «когда переписывать»; до ADR в код ECS не класть. Рекомендуемый минимум roadmap: `EntityId` + generation и `ComponentStore` для Transform/Renderable/Collider, без archetype/job system.

Depends: [[define-asset-folder-load-stub|feat: Asset registry and first load vertical]]. Next: [[feat-render-world-packets|feat: RenderWorld packets and instrumentation]]. Взято в [[Sprint 7 — Engine architecture]].

Origin: [[Architecture]] open question + merged `docs/architecture-roadmap.md` (2026-09-01).

ADR: [[ADR-012 Entity model EntityId and ComponentStore]] (Accepted). `EntityId` + `ComponentStore` in `rat_core`; player/events/`SimulationSession` not migrated.

## Resolution

ADR-012: hybrid minimum — sparse `EntityId` + generation and `ComponentStore` for Transform/Renderable/Collider, not archetype ECS or scene-graph identity. Stale-id tests in `[entity]`. Player/events stay POD. Verify: `.\build\tests\rat_tests.exe "[entity]"` and `ctest`. Review: Approved.

## Bugs found

none.
