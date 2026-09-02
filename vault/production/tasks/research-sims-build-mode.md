---
type: task
area: Engine
status: In progress
task_type: Research
sprint: Sprint 8
due:
tags: [task]
---

# research: Sims build-mode edit analog

Intent: зафиксировать, что брать из The Sims (стены по ребру, кисть пола, один stroke = один undo), что не брать (каталог объектов, этажи как в Sims). Нужно для нарезки Edit.

Acceptance: короткая заметка в vault (5–10 пунктов: wall tool, floor/terrain brush, drag along edges, cancel/undo). Без кода.

Origin: [[feat: Sims-like edit brush and edge paint]]. Related: [[Collect 5 reference games]]. Взято в [[Sprint 8 — Play feel and authoring]].

## Findings — The Sims build mode → rat-engine Edit

**Берём (аналог Sims, не клон):**

- **Стены по кликнутому ребру клетки** — инструмент Fence/Ramp/Ladder ставит барьер на ту грань N/E/S/W, которую пользователь попал курсором, а не на скрытый ImGui-combo default (сейчас [[edit-edges-only-one-facing]]). Слайс: [[feat: Edit paint clicked edge]].
- **Drag вдоль линии стены** — зажать ЛКМ на ребре и вести по смежным рёбрам той же «стены»; каждое пройденное ребро upsert/remove в том же stroke. Один непрерывный жест = одна группа в `EditHistory`.
- **Кисть пола/terrain по клеткам** — зажать ЛКМ и провести по height-grid: Place cube, slab, flat raise/lower красят каждую клетку под курсором (Sims floor/terrain brush). Слайс: [[feat: Edit hold-drag brush]].
- **Один mouse stroke = один undo** — press→drag→release (или abort) группирует все мутации stroke в одну команду/группу `EditHistory`; Ctrl+Z откатывает весь жест, не по клетке. Уже есть стек ([[feat: Edit undo/redo command stack]]); нужен `begin_group`/`end_group` или composite command на stroke.
- **Cancel / ПКМ прерывает stroke** — пока кнопка зажата, preview/live paint; отпускание ЛКМ коммитит группу; ПКМ или Esc до release отменяет stroke без записи в историю (Sims «abort placement»).
- **Инструмент = активный режим Edit** — как в Sims build: выбран Place cube / Fence / Slab определяет, что рисует drag; Play по-прежнему не пишет историю ([[feat: Mouse viewport terrain edit]]).
- **Pick по viewport, не по панели** — unproject + resolve cell/edge тот же pipeline, что [[feat: Mouse viewport map edit]]; ImGui остаётся для пресета (Mini/Full) и высоты, не для выбора стороны.

**Не берём (вне скоупа Sprint 8 / rat-engine):**

- Каталог объектов и buy mode — у нас blockers/events отдельно, не Sims furniture picker.
- Этажи как отдельные «истории» Sims (foundation + floor levels UI) — rat-engine: один height-grid + [[floor slab]] / ladder, без per-story camera stack.
- Plumbing, utilities, roof tool, terrain sculpt beyond cell paint — не greybox Edit.
- Auto-room detection, wall height variants per room, diagonal walls — только axis-aligned grid edges.

**Follow-up:** [[feat: Edit hold-drag brush]], [[feat: Edit paint clicked edge]] (порядок в [[feat: Sims-like edit brush and edge paint]]).
