---
type: bug
area: Engine
status: Fixed
severity: High
sprint: Sprint 14
tags: [bug]
---

# Cannot climb onto grey_yard bridge

Origin: playtest 2026-09-04 after [[Sprint 13 — Grey yard map pass]] / [[feat-grey-yard-layout-pass|feat: Grey yard layout pass]]. Related: [[feat-walkable-bridges-over-open-ground|feat: Walkable bridges over open ground]]. Follow-up: [[feat-voxel-3d-terrain-and-rotating-ramps|feat: Voxel 3D terrain and rotating ramps]]. Depends: [[feat-rotating-ramp-voxels|feat: Rotating ramp voxels]]. Origin: [[Sprint 14 — Voxel 3D terrain]]. Follow-up: [[chore-bridge-climb-review-test-polish|chore: Bridge climb review test polish]].

Occupancy-ramp high side omit’ит fence у occupancy solid и у abutting `floor_slab` (иначе crest на плиту не выходит).

Мост `(6–8, 5)` — airborne `floor_slabs` (`top_y` 2.0). Сверху ходить можно, снизу пройти можно. Заход с земли — две occupancy-рампы.

## Repro

1. Play `grey_yard`.
2. Подойти к мосту с земли (под плитами) или с лофта (дырка `(2,5)`, дальше три клетки пустоты).
3. Попытаться взойти на верх пролёта.

## Expected

Игрок может попасть на верх моста authored путём (рампа / лестница / воксельный терейн).

## Actual

Цилиндр остаётся на земле или на лофте; на `top_y` 2.0 не залезть.

## Resolution

Schema 5: occupancy-рампы `(4,0,5)` и `(5,1,5)` East к плитам `(6–8, 5)` `top_y` 2.0. Пролёт не заменён кубами 1 м; walk-under цилиндра 1.6 сохранён. Slab side fence на high-side рампы omit. Loft→мост не авторён (по брифу необязательно). Review: Approved.

Verify: Play `grey_yard`, с запада по двум клиньям на мост; под пролётом пройти. `.\build\tests\rat_tests.exe "[map],[collision],[player],[quest]"`.

## Bugs found

none.
