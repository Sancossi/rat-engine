---
type: bug
area: Engine
status: Fixed
severity: Medium
tags: [bug]
notion_id: 3cdf3827-36cc-815e-b626-c4e464909a1f
---

# Camera switch changes WASD world directions

## Repro

Move with WASD, switch TopDown/Tilt45/3/4 via C, press same key: world heading changes.

Expected: same key preserves world direction across camera switches.

Actual: camera-relative projected axes rotate in 3/4.

Decision: editor movement uses stable world mapping W=-Z, D=-X for all camera modes.

Follow-up: Play walk is camera-relative again — [[feat: Camera-aligned walk]]. `C` rotating W in Play is intended. Edit still has no player WASD.
