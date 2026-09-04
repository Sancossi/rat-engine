---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 12
due:
tags: [task]
---

# feat: Play walks event graph

Intent: `EventRuntime` шагает по `page.graph`, не по `commands[]`. Yield Wait / Set Move Route на ноде; branch — then/else рёбра.

Acceptance: `start_page` требует graph (loader уже заполнил). StackFrame = node id, не command index. Parallel budget считает ноды. `[event],[route],[quest]` зелёные. Не снимать `commands` из JSON в этом срезе. Не Ruby/JS VM.

Depends: [[feat: Reverse-compile commands to graph]]. Origin: [[Sprint 12 — Graph is Play truth]]. Related: [[ADR-014 Play executes event graphs]]. Follow-up: [[chore: Drop event page commands]], [[feat: Event graph in-node widgets]].

Spec: `docs/superpowers/specs/2026-09-04-play-walks-event-graph-design.md`
