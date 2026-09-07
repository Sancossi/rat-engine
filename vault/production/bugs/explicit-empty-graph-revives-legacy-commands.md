---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 15
tags: [bug, stabilization]
---

# Explicit empty graph revives legacy commands

Origin: [[s15-runtime]] — read-only replay and graph-authority review.

## Evidence

ensure_page_graph_from_commands can replace an explicitly present empty graph using stale page.commands. Graph-backed pages must not resume legacy behavior after the last node is removed. Runtime capability validation must also ignore stale commands when a graph is explicitly present.

## Acceptance

Only an absent graph triggers legacy migration. An explicitly empty graph remains empty and receives graph validation errors where appropriate. A regression proves old commands are not resurrected. Resolve with the approved graph-authority/runtime contract stage.


## Resolution

Fixed in eed6158 with reviewed append correction 4eaeaf6. Independent review Approved; full verification 689/689 C++ tests passed and reviewer focused run 19 tests/346 assertions passed. See [[s15-runtime]]. Real UI acceptance remains [[s15-acceptance]].

## Bugs found

No unresolved stage-4 findings.
