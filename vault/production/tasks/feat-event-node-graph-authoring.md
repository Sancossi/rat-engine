---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Event node graph authoring

Epic / parent. Слайсы (порядок):

1. [[research-event-node-graph|research: Event node graph vs bytecode]]
2. [[feat-event-graph-model-and-compile|feat: Event graph model and compile]]
3. [[feat-event-graph-editor-canvas|feat: Event graph editor canvas]]

Intent: авторство событий как **дерево/граф нод** (визуальное программирование), не только RM-список команд. Runtime остаётся data-driven interpreter ([[Event System]], [[ADR-007 Events and maps stored as JSON]]): граф **компилируется** в pages + `Command` list. Не Ruby/JS VM.

Acceptance: см. слайсы. MVP без полного MZ-набора команд.

Origin: chat 2026-09-02. Related: [[s2-event-inspector-pages-stub]], [[feat-play-se-event-command|feat: PlaySE event command]]. Взято в [[Sprint 8 — Play feel and authoring]].

Follow-up: [[chore-event-graph-join-golden-and-branch-edges|chore: Event graph join golden and branch edges]]

## Resolution

Все три слайса Done: граф только authoring, Edit компилирует в `commands`, Play тот же interpreter, ImGui-холст MVP в Edit. Verify: Apply edited map после правки графа; Play не читает `graph`.

## Bugs found

none.
