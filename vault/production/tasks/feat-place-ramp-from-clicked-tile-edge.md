---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 13
due:
tags: [task]
---

# feat: Place ramp from clicked tile edge

Intent: на платформу нужно заходить **с разных сторон** — несколько рамп на разных гранях, не одна east из combo. Сейчас Place ramp — ImGui `ramp_direction_index` (хвост [[feat: Edit paint clicked edge]]). Не открывать боковой заход на **одну** рампу ([[Ramp allows entry from side]] остаётся Fixed).

Acceptance:

- Place ramp в viewport: грань N/E/S/W = `nearest_tile_edge`, как Fence/Ladder. Противоположная сторона той же клетки — отдельная рампа на соседе или flip без охоты по combo.
- Две рампы на разные подходы одной высокой клетки (например west и south) — обе рабочие в Play.
- Combo в панели может остаться override. Не imgui-node-editor.

Origin: chat 2026-09-04 (Sprint 13, рампы с разных сторон). Origin: [[feat: Edit paint clicked edge]] (ramp facing leftover). Origin: [[Sprint 13 — Grey yard map pass]]. Related: [[edit-edges-only-one-facing]].

## Resolution

## Bugs found
