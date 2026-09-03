---
type: task
area: Engine
status: In review
task_type: Research
sprint: Sprint 10
due:
tags: [task]
---

# research: Set Move Route

Intent: GDD / [[Event System]] v1 ещё держит Set Move Route (basic). Нужно решить, как ходить NPC по сетке без physics-ящиков: формат команды, кто тикает (Parallel vs interpreter), коллизия с игроком. Не писать runtime в этом слайсе.

Acceptance: заметка take / don't take на этой карточке (как [[research: Sims build-mode edit analog]]). Следующий спринт сможет нарезать impl. Не Event touch, не party follow.

Origin: [[Define vertical slice scope]] Out; [[Event System]] commands v1. Взято в [[Sprint 10 — Grey yard content]].

## Findings — Set Move Route → rat-engine

**Берём:**

- **Ходит runtime overlay, не authored `EventDef`.** `EventDef.tile` / `volume` / `y` остаются спавном в JSON и Edit. `EventRuntime` держит `event_id → {tile, facing}` (live x/z на сетке). `event_bounds`, Action-радиус, Player touch и greybox markers читают overlay, иначе fallback на `EventDef`. Save не записывает патруль; hot-apply сбрасывает overlay вместе с `load()`. Volume-only события в v1 не ходят (skip + warn). Гибрид [[ADR-004 Hybrid map movement and events]]: игрок свободен, NPC — tile-step.
- **Одна команда `set_move_route` в page `commands[]`, шаги во вложенном `route`.** Не расплющивать `move`/`turn` в список page (это не Set Move Route из [[Event System]] / [[GDD]]). Не класть шаги в `then_commands` (это Conditional Branch). `CommandOp` сегодня без этого op — появится в impl. JSON:

```json
{
  "op": "set_move_route",
  "through": false,
  "route": [
    { "op": "move", "dir": "east" },
    { "op": "move", "dir": "east" },
    { "op": "wait", "frames": 20 },
    { "op": "turn", "dir": "south" }
  ]
}
```

  v1 steps: `move` N/E/S/W на одну клетку, `wait`, `turn`. Target только **this** (событие текущего interpreter). Always wait-until-done: interpreter не идёт дальше, пока `route` не кончится.
- **Тикает тот же page interpreter, yield как `Wait`.** Не второй VM и не nested Parallel ([[ADR-008 Parallel and Autorun runtime limits]]: 8 / 32 / no nested). `exec_command` ставит курсор по `route`; каждый кадр — один шаг или `wait_frames` на клетку (константа impl, не RM speed). Parallel-бюджет: opcode считается как `Wait` (не N команд JSON за кадр). **Action / Autorun / PlayerTouch** — foreground, игрок залочен, пока NPC идёт (катсцена). **Parallel** — игрок свободен; патруль = Parallel page, цикл через уже существующий restart, когда interpreter finished. Из Action нельзя «запустить Parallel, чтобы пройтись».
- **Коллизия без ECS-тел.** Перед шагом: клетка на сетке; AABB клетки vs `PlayerBody` (как `player_overlaps`); blockers через `blocker_blocks_feet` + AABB; заборы/стены — ephemeral `CollisionBody` в стеке против уже baked `CollisionWorld` (`cylinder_hits_fences` / `cylinder_hits_walls`). Не `ComponentStore`, не persistent collider на событии ([[ADR-012 Entity model EntityId and ComponentStore]] игрока и events не мигрирует). Занято → ждать кадр (не skip, не обход). `through: true` игнорирует игрока/blockers/fences, сетку карты не покидает.

**Не берём (вне v1 / не этот слайс):**

- Event touch (enum есть, `why_not` → WrongPage; для подвижных NPC позже).
- Party follow / move player / move other event.
- Pathfinding вокруг ящиков; skippable «пропустить шаг если блок».
- Jump / climb / ramp-as-route; диагонали.
- Set Move Route как второй interpreter или nested Parallel из Action.
- Repeat-флаг на команде (патруль = Parallel page loop).
- Graph-нода Move Route в MVP canvas ([[research: Event node graph vs bytecode]]).
- Event↔event коллизия (два NPC на одной клетке).

**Follow-up:** [[feat: Set Move Route (basic)]] — следующий спринт, не Sprint 10. `CommandOp::SetMoveRoute` + parse/serialize `route[]` + overlay + yield + probe коллизии + markers из overlay. Карту `grey_yard` и Art Direction не трогать в том слайсе без отдельной content-карточки.
