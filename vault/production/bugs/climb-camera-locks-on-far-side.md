---
type: bug
area: Engine
status: Open
severity: High
sprint: Sprint 8
tags: [bug]
---

# Climb camera locks on the far side of the ladder

Origin: chat 2026-09-02 playtest after [[feat: MGS3 ladder climb]]. Test map `grey_yard`: East ladder on tile `(2, 4)`, approach from `(3, 5)`. Взято в [[Sprint 8 — Play feel and authoring]].

## Repro

1. Play `grey_yard`.
2. Approach the East ladder at `(2, 4)` from the east / from `(3, 5)` (the intended grab side of that face).
3. Press Interact, climb.

## Expected

Camera sits **behind the player**, looking at the rungs. Screen-forward is up the ladder.

## Actual

Lock uses `ladder_face_into` (East = +X): eye = player − into × 10. For an East face approached from **+X**, that puts the camera on the **west / loft side** of the wall — the far side, not behind the climber.

`climb_into_*` is the owner-tile into vector, not “from player toward rungs”. W = up is tied to the same vector, so a camera flip must keep that dot.

Lock also **snaps**; want a smooth turn: [[feat: Smooth camera turn]].

Follow-up from: [[feat: MGS3 ladder climb]]
