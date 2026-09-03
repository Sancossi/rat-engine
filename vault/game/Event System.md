---
type: note
tags: [game, events]
---

# Event System

> **Status:** Working (2026-08-31). RM-inspired, не клон. Данные data-driven. Правка в playmode — цель vertical slice.

## Цели

- Авторы описывают квесты/диалоги/загадки событиями, а не C++.
- Поведение предсказуемо: pages + conditions + commands.
- Триггеры совместимы с **гибридной картой** (клетка + volume).

## Сущности

- **Map** — сцена с сеткой-помощником и свободным движением
- **Event** — объект с позицией (tile и/или transform), списком **Pages**
- **Page** — conditions + trigger + command list + graphic/optional collider
- **Game State** — switches, variables, self-switches, items, party pos

## Triggers (MVP)

| Trigger | Когда |
| --- | --- |
| Action | Игрок нажал interact, глядя/стоя у события |
| Player touch | Игрок вошёл в tile/volume |
| Event touch | Событие коснулось игрока (для подвижных) |
| Autorun | Старт page при выполнении conditions (блокирует до конца) |
| Parallel | Фоновый процесс (ограничить бюджет) |

## Conditions (MVP)

- Switch ON/OFF
- Variable (>=, ==, …)
- Has item
- Self-switch A–D

## Commands v1

Show Text, Control Switch, Control Variable, Conditional Branch, Set Move Route (basic), Transfer Player, Change Items, Wait, Comment / Label+Jump (optional later)

## Edit-in-playmode

- Создать/удалить event
- Править pages, conditions, command list
- Привязка к tile или volume gizmo
- Apply без полного рестарта карты (hot apply); hard reload — fallback

## Non-goals (пока)

- Полный Ruby/JS scripting runtime
- Все команды MV/MZ
- Battle Processing (нет turn-based battle screen)

## Parallel / Autorun limits (MVP)

| Лимит | Значение |
| --- | --- |
| Активных Parallel на карте | **8** |
| Команд Parallel за кадр (суммарно все) | **32** |
| Одновременных Autorun | **1** (блокирует вход игрока в обычный control до конца) |
| Nested Parallel из Parallel | **запрещён** в v1 |
| Busy-loop без Wait | **запрещён** (нужен Wait / yield) |
| При превышении | skip + **warn/log** (в Edit — заметный toast/console) |

См. [[ADR-007 Events and maps stored as JSON]], [[ADR-008 Parallel and Autorun runtime limits]].

## Open questions

- [x] Формат хранения → **JSON**
- [x] UX команд → **RM-like list** (графический граф — бэклог [[feat: Event node graph authoring]])
- [x] Лимиты Parallel → **8 / 32 / no nested**
- [x] Set Move Route (basic) → runtime overlay + `set_move_route.route[]` + тот же interpreter yield (как Wait); не второй VM. См. [[research: Set Move Route]]
