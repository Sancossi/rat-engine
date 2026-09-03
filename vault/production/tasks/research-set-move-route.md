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

- **Ходит runtime overlay, не authored `EventDef`.** `EventDef.tile` / `volume` / `y` остаются спавном в JSON и Edit. `EventRuntime` держит `event_id → {tile, facing}` (live x/z на сетке). Overlay-клетка побеждает; tile+volume live bounds = volume, сдвинутый на тот же tile-delta, что `translate_event_on_grid`. Volume-only в v1 не ходят (skip + warn). Dest-клетка семплит surface/solids для ног Y и markers; `EventDef.y` — только спавн. `event_bounds`, Action-радиус и greybox markers читают overlay, иначе fallback на `EventDef`. PlayerTouch стартует только когда игрок сам заходит в live bounds; overlap из-за overlay (NPC наступил на стоящего игрока) PlayerTouch не запускает. Страница PlayerTouch может содержать `set_move_route`. Save не записывает патруль; hot-apply сбрасывает overlay вместе с `load()`. Гибрид [[ADR-004 Hybrid map movement and events]]: игрок свободен, NPC — tile-step.
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
- **Тикает тот же page interpreter, yield как `Wait`.** Не второй VM и не nested Parallel ([[ADR-008 Parallel and Autorun runtime limits]]: 8 / 32 / no nested). `command_index` остаётся на `set_move_route`, пока `route` не кончится; курсор шагов — `route_index` + `wait_frames` на том же opcode (не stack walk Conditional Branch, не `CommandOp::Move` в page enum). Каждый кадр — один шаг или `wait_frames` на клетку (константа impl, не RM speed). Parallel-бюджет: opcode считается как `Wait` (не N команд JSON за кадр). **Action / Autorun / PlayerTouch** — foreground, игрок залочен, пока NPC идёт (катсцена). **Parallel** — игрок свободен; патруль = Parallel page, цикл через уже существующий restart, когда interpreter finished. Из Action нельзя «запустить Parallel, чтобы пройтись».
- **Коллизия без ECS-тел.** Перед шагом: клетка на сетке; игрок vs dest tile через `circle_overlaps_aabb2` (как `player_overlaps`, не AABB vs AABB); blockers через `blocker_blocks_feet` + AABB; заборы/стены — ephemeral `CollisionBody` в стеке в центре dest-клетки против baked `CollisionWorld` (`cylinder_hits_fences` / `cylinder_hits_walls`). Не `ComponentStore`, не persistent collider на событии ([[ADR-012 Entity model EntityId and ComponentStore]] игрока и events не мигрирует). **Foreground (Action / Autorun / PlayerTouch):** занятость игрока = through (залоченный игрок не может сойти с dest). Blockers/fences по-прежнему блокируют, пока не `through: true`. **Parallel** ждёт кадр на занятой клетке (игрок может отойти). Не skip-after-N. `through: true` игнорирует игрока/blockers/fences, сетку карты не покидает.

**Не берём (вне v1 / не этот слайс):**

- Event touch (enum есть, `why_not` → WrongPage; overlap из-за overlay PlayerTouch не стартует).
- Party follow / move player / move other event.
- Pathfinding вокруг ящиков; skippable «пропустить шаг если блок».
- Jump / climb / ramp-as-route; диагонали.
- Set Move Route как второй interpreter или nested Parallel из Action.
- Repeat-флаг на команде (патруль = Parallel page loop).
- Graph-нода Move Route в MVP canvas ([[research: Event node graph vs bytecode]]).
- Event↔event коллизия (два NPC на одной клетке).

**Follow-up:** [[feat: Set Move Route (basic)]] — следующий спринт, не Sprint 10. `CommandOp::SetMoveRoute` + parse/serialize `route[]` + overlay + yield + probe коллизии + markers из overlay. Карту `grey_yard` и Art Direction не трогать в том слайсе без отдельной content-карточки.
