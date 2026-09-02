---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Event node graph authoring

Epic / parent. Слайсы (порядок):

1. [[research: Event node graph vs bytecode]]
2. [[feat: Event graph model and compile]]
3. [[feat: Event graph editor canvas]]

Intent: авторство событий как **дерево/граф нод** (визуальное программирование), не только RM-список команд. Runtime остаётся data-driven interpreter ([[Event System]], [[ADR-007 Events and maps stored as JSON]]): граф **компилируется** в pages + `Command` list. Не Ruby/JS VM.

Acceptance: см. слайсы. MVP без полного MZ-набора команд.

Origin: chat 2026-09-02. Related: [[s2-event-inspector-pages-stub]], [[feat: PlaySE event command]]. Взято в [[Sprint 8 — Play feel and authoring]].
