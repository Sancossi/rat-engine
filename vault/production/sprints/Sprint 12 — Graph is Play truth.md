---
type: sprint
status: In progress
dates: 2026-09-04/2026-09-18
goal: Play executes event graphs; nodes are editable cards; commands[] leave the runtime
current: true
tags: [sprint]
---

# Sprint 12 — Graph is Play truth

Цель: источник истины page-body — граф. Play ходит по нодам; ноды настраиваются на карточке, не списком `commands`.

DoD:

- Loader поднимает legacy `commands` в `graph`
- `EventRuntime` шагает по графу (Wait / Move Route yield, branch then/else)
- Save не требует command list; схема обновлена
- Canvas: виджеты внутри ноды; RMB add/copy/delete, drag-связи, хоткеи undo/redo

- `[event],[route],[quest],[graph]` зелёные

Вне скоупа: меню выбора / портреты / fade из референс-скрина, Label+Jump, Event touch, Ruby/JS, imgui-node-editor.

Порядок:

- [[feat: Reverse-compile commands to graph]] → [[feat: Play walks event graph]] → [[chore: Drop event page commands]] → [[feat: Event graph in-node widgets]] → [[feat: Event graph RMB add copy delete and wiring]]

Spec: `docs/superpowers/specs/2026-09-04-play-walks-event-graph-design.md`

ADR: [[ADR-014 Play executes event graphs]]
