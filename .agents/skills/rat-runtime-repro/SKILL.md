---
name: rat-runtime-repro
description: Reproduce rat-engine simulation and editor defects with focused fixtures, input sequences, and evidence. Use for runtime regressions and acceptance scenarios.
---

# Reproduce runtime defects

Read the affected card and the existing tests before constructing a fixture.
SimulationSession is the shared simulation entrypoint; inspect `tests/` examples
for the relevant input sequence, map loading, event runtime, and replay contracts.
Use a temporary copy when a scenario saves or edits a map; never overwrite user content.

Capture initial map, player state, simulation configuration, exact per-tick input/dt,
expected outcome and observed divergence. Prefer the smallest fixture that preserves
the failure; retain a grey_yard integration scenario when the bug depends on its geometry.
Assert state changes and invariants rather than calling the same helper twice.

F3 debug snapshots and screenshots support desktop reproduction. Headless checks
cannot establish focus, dragging, modal or rendering behavior. Keep those claims
explicitly unverified until a real editor-input scenario has run. A replay checksum
only proves fields included by its current contract; inspect that contract before
using replay success as evidence. Record residual failures on the source card with
linked followups according to `AGENTS.md`.
