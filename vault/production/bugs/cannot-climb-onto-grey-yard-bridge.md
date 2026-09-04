---
type: bug
area: Engine
status: Open
severity: High
sprint:
tags: [bug]
---

# Cannot climb onto grey_yard bridge

Origin: playtest 2026-09-04 after [[Sprint 13 — Grey yard map pass]] / [[feat: Grey yard layout pass]]. Related: [[feat: Walkable bridges over open ground]]. Follow-up: [[feat: Voxel 3D terrain and rotating ramps]].

Мост `(6–8, 5)` — airborne `floor_slabs` (`top_y` 2.0). Сверху ходить можно, снизу пройти можно, **забраться с земли/лофта нельзя**: нет рампы и нет лестницы на пролёт. Layout pass сознательно не ставил loft→мост лестницу.

## Repro

1. Play `grey_yard`.
2. Подойти к мосту с земли (под плитами) или с лофта (дырка `(2,5)`, дальше три клетки пустоты).
3. Попытаться взойти на верх пролёта.

## Expected

Игрок может попасть на верх моста authored путём (рампа / лестница / воксельный терейн).

## Actual

Цилиндр остаётся на земле или на лофте; на `top_y` 2.0 не залезть.
