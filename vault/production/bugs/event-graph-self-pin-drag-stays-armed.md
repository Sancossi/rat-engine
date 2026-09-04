---
type: bug
area: Engine
status: Open
severity: Low
sprint:
tags: [bug]
---

# Event graph self-pin drag stays armed

Origin: [[feat: Event graph RMB add copy delete and wiring]] (review of `bffb449`).

## Repro

1. Event Graph: drag a wire from a node's out-pin onto **that same node's** in-pin and release.

## Expected

Connect is rejected (`from == to`); drag state clears; no preview bezier.

## Actual

`connect_event_graph_nodes` fails, but `dragging_wire` can stay true until Esc/RMB/next release, so the preview hangs.
