---
type: sprint
status: Done
dates: 2026-09-01/2026-09-14
goal: Shared human/agent debug loop plus GPP Command, Event Queue, Observer, State
current: false
tags: [sprint]
---

# Sprint 4 — Refactoring and AI workflow

Цель: человек и агент видят одно текстовое состояние runtime; GLFW не торчит в геймплей; ближний слой GPP (Command / Event Queue / Observer / State) лежит в `rat_core`, не в GLFW-shell.

DoD:

- лог в файл/`stderr` без синглтона
- JSON snapshot кадра (F3 + API)
- why-not-fired для event page
- headless прогон последовательности ввода
- research-заметка по loopback MCP (без установки чужих MCP)
- `InputFrame` вместо сырого `glfwGetKey` в геймплее
- очередь `Audio` (null/log sink) из composition root, без locator
- opcode PlaySE постит в очередь
- шина gameplay notify без синглтона
- undo/redo для Edit blockers/events
- locomotion FSM (Idle/Walk/Jump/Fall); физика прыжка не в FSM

Вне скоупа: мышь в Edit, ECS, object pool, spatial partition, ребинд/геймпад, audio backend ADR, незакрытый height-grid polish.

Порядок:

- Агент: [[logging-and-assert-helpers|Logging and assert helpers]] → [[feat-input-action-mapping|feat: Input action mapping]] → [[feat-debug-snapshot-json|feat: Debug snapshot JSON]] → [[feat-event-why-not-fired|feat: Event why-not-fired]] → [[feat-headless-input-sequence-probe|feat: Headless input sequence probe]] → [[research-rat-debug-loopback-mcp|research: Rat debug loopback MCP]]
- GPP: [[feat-input-action-mapping|feat: Input action mapping]] → [[feat-audio-play-queue-stub|feat: Audio play-queue stub]] → [[feat-play-se-event-command|feat: PlaySE event command]] / [[feat-gameplay-notify-observer|feat: Gameplay notify observer]]; параллельно [[feat-edit-undo-redo|feat: Edit undo/redo command stack]] и [[feat-player-locomotion-fsm|feat: Player locomotion FSM]]

## Итог

Sprint закрыт 2026-08-31. DoD выполнен: лог без синглтона, JSON snapshot, why-not-fired (включая start-lock `ok`), headless probe, заметка loopback MCP (остаёмся на файлах), `InputFrame`, audio queue, PlaySE, notify bus, undo/redo Edit, locomotion FSM. Follow-up polish с ревью — Done. Стены height-grid в Sprint 4 не брали → [[Sprint 5 — Terrain and edge walls]].
