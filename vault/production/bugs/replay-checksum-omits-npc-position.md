---
type: bug
area: Engine
status: Open
severity: Medium
sprint:
tags: [bug, audit]
---

# Replay checksum omits NPC position

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A07.
Подробности и источники: [отчёт](../../../docs/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: карты с тем же ID и маршрутами east/west дают разные позиции NPC при одинаковом checksum.

## Expected / acceptance

Checksum включает overlay и продвижение VM/route; политика fingerprint карты определена.
