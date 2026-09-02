---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Sims-like edit brush and edge paint

Intent: переделать Edit mode ближе к The Sims build. Зажать кнопку и вести — кисть по нескольким клеткам (куб / высота / плита). Стены (рамки) — по **кликнутому ребру** клетки, drag вдоль стены, все четыре стороны без combo-по-умолчанию.

Acceptance: hold-drag applies the active tool to every cell/edge the cursor crosses (one undo group). Click near a tile edge places/removes that facing (N/E/S/W). Reference: Sims wall/floor paint, not RM tile picker. Does not replace Play.

Origin: chat 2026-09-02. Related: [[edit-edges-only-one-facing]], [[feat: Mouse viewport terrain edit]], [[Collect 5 reference games]].
