---
type: sprint
status: Done
dates: 2026-09-04/2026-09-04
goal: Play executes event graphs; nodes are editable cards; commands[] leave the runtime
current: false
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

- [[feat: Reverse-compile commands to graph]] → [[feat: Play walks event graph]] → [[chore: Drop event page commands]] → [[feat: Event graph in-node widgets]] → [[feat: Event graph RMB add copy delete and wiring]] → [[feat: Cyrillic in texts and comments]] → [[Ramp greybox fill breaks cells and hides the high side]] → [[Event on ramp is not mouse-clickable in Edit]]

Spec: `docs/superpowers/specs/2026-09-04-play-walks-event-graph-design.md`

ADR: [[ADR-014 Play executes event graphs]]

## Итог

DoD выполнен 2026-09-04. Play ходит по `page.graph`; save без `commands`; canvas — виджеты на ноде, RMB add/copy/delete, drag-связи, undo; кириллица в Show Text / Comment (Noto Sans). Playtest: сетка рампы по углам клетки; клик ивента на рампе через `unproject_to_terrain`.

## Bugs found

на закрытии: [[Event graph self-pin drag stays armed]] (Low, в бэклог). Playtest-фиксы в этом спринте: [[Ramp greybox fill breaks cells and hides the high side]], [[Event on ramp is not mouse-clickable in Edit]] — Fixed.
