---
type: task
area: Engine
status: In review
task_type: Chore
sprint: Sprint 8
due:
tags: [task]
---

# chore: Event graph join golden and branch edges

Intent: закрыть leftover компилятора после [[feat: Event graph model and compile]] / canvas review: join-after-branch и sequence-ребро с `conditional_branch`.

Acceptance: Catch2 golden — then/else сходятся в общий узел, затем он продолжается; sequence-ребро из branch → compile error, не молчаливый drop. Canvas не обязан auto-splice.

Origin: [[feat: Event graph editor canvas]] review (Approved, Minor). Related: [[feat: Event graph model and compile]].

Implementer `9c2424c`: join golden + sequence-from-branch compile error. Review pending.
