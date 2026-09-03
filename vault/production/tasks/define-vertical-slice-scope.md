---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 9
roadmap: Content vertical slice
due:
tags: [task]
notion_id: 3ccf3827-36cc-8111-94f9-f8c8ee497873
---

# Define vertical slice scope

Intent: зафиксировать одну короткую сессию (5–15 мин) на `grey_yard` (или явном наследнике): что в срезе, что нет, как пройти руками.

Acceptance: заметка на этой карточке — карта, квест, NPC, field action, edit-in-playmode demo, out-of-scope. Опереться на [[Collect 5 reference games]]. Не писать код и не плодить новый контент-спринт.

Related: [[GDD]], [[S1: Acceptance playthrough checklist]], [[Sprint 9 — First playable loop]], [[Content vertical slice]], [[ADR-006 Edit-in-playmode author loop]].

## Locked

| | |
| --- | --- |
| **Map** | `grey_yard` (`data/maps/grey_yard.json`). Не наследник. |
| **Session** | 5–15 мин в `rat-editor`, Play. Один двор, не вторая карта и не 3-room dungeon. |
| **Quest** | 1 NPC (foreman) + scrap/`rusty_cog` на switches 1–2. GDD «2–3 NPC» — будущий контент **на этой же** карте, не fork filename. |
| **Field action** | То, что уже на карте: Climb лестницы на лофт + прыжок через jumpable на gantry. Не «отпугнуть/сломать блокер». |
| **Edit demo** | Один жест: **F2 → перетащить crate-blocker → F2 Play**, короткий путь к scrap без рестарта. |

`grey_yard` — карта среза, пока её явно не заменим. Successor не нужен: двор, квест, лестница, плиты и gantry уже здесь; новый JSON не разблокирует DoD.

## Play path (beat-by-beat)

Честный проход: ядро = S1 cog; обход = лофт/gantry; persist = I + пауза; demo = один Edit-жест. Автотест ядра: `ctest` *grey_yard cog quest end-to-end*.

1. **Launch** — `rat-editor`, карта `grey_yard`. Autorun `yard_intro` (клетка 0,0): «Grey Yard — a steampunk workshop lot. Talk to the foreman.» Снять **E / Space**. **Уже есть.**
2. **Foreman** — cyan, ~(-2, 2), Action. Принять квест (switch 1): rusty cog для gate winch, scrap east of the crates. **Уже есть** (1 NPC; второго/третьего нет).
3. **Crates** — AABB (3–5, −1–1) режет короткий путь; volume `crate_notice` (Player touch, пока switch 2 выкл.): обойти north/south. **Уже есть.**
4. **Scrap** — ~(6, 0). До quest: «Ask the foreman». После switch 1: `rusty_cog`, self-switch A. Повтор: «picked clean». **Уже есть.**
5. **Inventory** — **I**: RM-список, виден `rusty_cog`. Повтор I закрывает. **Уже есть** ([[feat: Inventory list UI]]).
6. **Turn-in** — снова foreman (Has item): quest complete, cog списан, switch 2. После: «Yard's quiet». **Уже есть.**
7. **Loft (field action)** — лестница east на (2, 4), `y` 0–2; плиты `top_y` 2.0 на (0–2,4) и (0–1,5), дыра (2,5). Interact лицом → Climb, камера сзади, вперёд = вверх ([[feat: MGS3 ladder climb]]). Сход на плиту / в дыру. **Геометрия есть; события на плите нет** — это [[feat: Loft event height]] (не DoD Sprint 9). Ground-event с плиты не должен fire ([[events-match-ground-while-on-slab]], Fixed).
8. **Gantry (field action)** — рампа east (8, 8) 0→1; jumpable на ~y 1 у (10, 8); Action `elevated_after_blocker` (11, 8) → switch 40 «Relay lever». **Уже есть** как обход, **не** связан с cog. Прыжок — [[feat: Responsive Mario-like jump]] (Space), не scare-blocker из [[GDD]].
9. **Pause / save** — Esc: слот пишет `GameState` (карта, xyz, switches/vars/items) и грузит обратно. **DoD Sprint 9** ([[feat: Save load game file]]); на момент lock слайс ещё может идти параллельно — сессия считает это обязательным, не «уже в билде».
10. **Edit-in-playmode** — жест ниже, тот же exe, без рестарта ([[S2: Acceptance — edit then play without restart]]).

Уложиться в 5–15 мин: 1–6 ≈ 5 мин; 7–8 по желанию; 9–10 добирают верхнюю границу.

## Edit-in-playmode demo (один жест)

Рецензент в том же `rat-editor`:

1. После шага 2 (квест взят) или на свежем запуске: **F2** → Edit ([[S2: Play/Edit mode toggle]]; Tab занят ImGui).
2. Клик по crate-blocker (3–5 × −1–1) → drag по XZ земли, **с короткого пути** (например north, за z ≥ 1). [[feat: Mouse viewport map edit]].
3. **F2** → Play. Идти east от спавна к scrap **напрямую**, без обхода. Коллизия сразу.
4. (Не часть жеста.) Crate volume остаётся на старом AABB — это ок: demo про карту, не про синхрон volume.

Не в этом жесте: paint клетки, правка event page, F5 hot-apply, save карты Edit. GDD «проп + page» — page отдельно, после среза.

## Out of scope (эта сессия / Sprint 9)

Уже вычеркнуто спринтом: Set Move Route / NPC walk, полный MZ palette на холсте, field physics, moodboard, spatial partition / FX pool.

Дополнительно для vertical slice:

- **Successor map / вторая комната** — не плодим JSON; `grey_yard` держим.
- **2–3 NPC и scare-blocker** из [[GDD]] — не на текущем `grey_yard`; не блокер lock.
- **Событие на лофте (Y плиты)** — [[feat: Loft event height]]; не DoD Sprint 9.
- **Battle screen / combat-RPG** — [[GDD]] non-goal, [[ADR-005 Conflict model exploration plus field action]].
- **Meshes as required** — greybox / столбы ок ([[S1: Acceptance playthrough checklist]] Known issues).
- **[[Art direction moodboard pass]]** — палитры FF6 / Chrono Cross после среза, не в сессии.
- **[[feat: Field physics puzzles]]** — ящики / панели / провалы / двери с разбегу.
- **Set Move Route** — команда в [[Event System]] v1, runtime не в first playable.
- **Deep inventory** (экип, сетка, drag-drop) — только RM-список.

## Refs → эта сессия

По одной строке на игру из [[Collect 5 reference games]] (take той карточки; здесь только привязка к проходу):

| Game | Эта сессия |
| --- | --- |
| **Metal Gear Solid 3** | Beat 7: Interact → Climb на лестнице (2, 4), не «идти в объём»; камера сзади, вперёд = вверх. |
| **Final Fantasy 6** | Один двор = один scene lock; greybox читает силуэт квеста. Pixel/32px palette — moodboard, не DoD. |
| **Chrono Cross** | 3D-двор + потом pixel albedo; pre-render фон запрещён — иначе ломается Edit в том же exe (beat 10). |
| **Disco Elysium** | Beats 2/6: разговор = квестовый глагол (Show Text + switch / Has item), не бой и не роман. |
| **Divinity: Original Sin** | Beats 4–6: ветка foreman смотрит в инвентарь (`rusty_cog`); успех = другая page, не кубик. |

## Follow-up

- Контент 2-го NPC / scare-blocker — только если остаёмся на `grey_yard`, отдельная task, не эта карточка.
- [[feat: Loft event height]] — когда понадобится триггер на плите.
- [[Art direction moodboard pass]] — look-lock FF6 / CC после playable loop.

## Resolution

Срез = `grey_yard`, 5–15 мин: cog-квест (foreman + scrap/`rusty_cog` + I), Climb лофта, прыжок на gantry, Esc save, один Edit-жест (F2 → отодвинуть crate **на два тайла** north → F2 Play). GDD 2–3 NPC / scare-blocker — не в этом lock. Verify: пройти beats 1–6 и жест crate; читать эту карточку. Review: Approved.

## Bugs found

none.
