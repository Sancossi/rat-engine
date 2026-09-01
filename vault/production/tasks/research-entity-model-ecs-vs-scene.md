---
type: task
area: Engine
status: Not started
task_type: Research
sprint:
due:
tags: [task]
---

# research: Entity model ECS vs scene vs hybrid

Intent: закрыть open question в [[Architecture]] (GPP Component). Сейчас карта = POD-события + player + blockers, ECS не нужен. Спайк сравнивает ECS / scene-graph / hybrid «игрок+события как сейчас». Контекст: [[Game Programming Patterns]].

Триггер **не** коллизии и не [[feat: Field physics puzzles]]: несколько динамических тел (герой, ящики, двери) живут как массив `CollisionBody` без ECS. Переписывать имеет смысл, когда разнородных акторов с разными наборами компонент станет много, или когда сцена-граф начнёт дублировать то же самое в трёх местах. До ADR в код ECS не класть.

Acceptance: ADR (следующий номер после ADR-011) с выбором и критерием «когда переписывать»; до ADR в код ECS не класть.
