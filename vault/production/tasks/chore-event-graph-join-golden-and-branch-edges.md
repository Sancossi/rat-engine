---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 8
due:
tags: [task]
---

# chore: Event graph join golden and branch edges

Intent: закрыть leftover компилятора после [[feat-event-graph-model-and-compile|feat: Event graph model and compile]] / canvas review: join-after-branch и sequence-ребро с `conditional_branch`.

Acceptance: Catch2 golden — then/else сходятся в общий узел, затем он продолжается; sequence-ребро из branch → compile error, не молчаливый drop. Canvas не обязан auto-splice.

Origin: [[feat-event-graph-editor-canvas|feat: Event graph editor canvas]] review (Approved, Minor). Related: [[feat-event-graph-model-and-compile|feat: Event graph model and compile]].

## Resolution

Catch2 locks join-after-branch: then/else stop at the shared node; that node's commands follow the branch. A sequence edge out of `conditional_branch` is a `MapIssue` (`ok == false`), not a silent drop. Verify: `.\build\tests\rat_tests.exe "*join after*","*sequence edge*"`. Review: Approved.

## Bugs found

none.
