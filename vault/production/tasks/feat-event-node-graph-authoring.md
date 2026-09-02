---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Event node graph authoring

Intent: авторство событий как **дерево/граф нод** (визуальное программирование), не только RM-список команд. Runtime остаётся data-driven interpreter ([[Event System]], [[ADR-007 Events and maps stored as JSON]]): граф **компилируется** в pages + `Command` list. Не Ruby/JS VM.

Acceptance: Edit открывает граф выбранного event/page; nodes для trigger / conditions / commands / branches; save/load round-trip в JSON; Play выполняет тот же bytecode, что и сегодня. MVP без полного MZ-набора команд.

Origin: chat 2026-09-02. Related: [[Event System]], [[s2-event-inspector-pages-stub]], [[feat: PlaySE event command]].
