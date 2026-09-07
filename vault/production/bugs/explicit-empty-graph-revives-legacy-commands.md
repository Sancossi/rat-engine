---
type: bug
area: Engine
status: Investigating
review: In review
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
