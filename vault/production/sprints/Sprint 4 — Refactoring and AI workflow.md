---
type: sprint
status: Active
dates: 2026-09-01/2026-09-14
goal: Shared human/agent debug loop plus GPP Command, Event Queue, Observer, State
current: true
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

- Агент: [[Logging and assert helpers]] → [[feat: Input action mapping]] → [[feat: Debug snapshot JSON]] → [[feat: Event why-not-fired]] → [[feat: Headless input sequence probe]] → [[research: Rat debug loopback MCP]]
- GPP: [[feat: Input action mapping]] → [[feat: Audio play-queue stub]] → [[feat: PlaySE event command]] / [[feat: Gameplay notify observer]]; параллельно [[feat: Edit undo/redo command stack]] и [[feat: Player locomotion FSM]]
