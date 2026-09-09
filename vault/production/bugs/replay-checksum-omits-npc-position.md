---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 15
tags: [bug, audit]
---

# Replay checksum omits NPC position

Origin: [[project-audit-followups]] — ревизия 2026-09-07, A07.
Подробности и источники: [отчёт](../../../docs/archive/cpp/audits/2026-09-07-project-review.md).

## Repro / evidence

Probe: карты с тем же ID и маршрутами east/west дают разные позиции NPC при одинаковом checksum.

## Expected / acceptance

Checksum включает overlay и продвижение VM/route; политика fingerprint карты определена.


## Resolution

Fixed in eed6158 with reviewed append correction 4eaeaf6. Independent review Approved; full verification 689/689 C++ tests passed and reviewer focused run 19 tests/346 assertions passed. See [[s15-runtime]]. Real UI acceptance remains [[s15-acceptance]].

## Bugs found

No unresolved stage-4 findings.
