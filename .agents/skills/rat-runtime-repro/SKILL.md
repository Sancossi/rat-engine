---
name: rat-runtime-repro
description: Reproduce Rat Expedition Stride gameplay and editor defects with focused fixtures, input sequences, and evidence. Use for runtime regressions and acceptance scenarios.
---

# Reproduce runtime defects

Read the affected card, current traversal specification and Core scenarios in
`games/rat-expedition/Rat.Expedition.Core.Tests/` before constructing a fixture.
Inspect `SceneDefinition`, `TraversalMotor` and the input-to-fixed-tick path in
`ExpeditionGame`. Graphics and platform behavior belong to the Windows application;
Core scenarios run without a GPU. Copy scene content before testing invalid data
or edits; never overwrite user content.

Capture scene schema/id, initial motor state, tick rate, exact inputs/dt, expected
outcome and observed divergence. Prefer a small fixture that reproduces observable
behavior; keep courtyard integration coverage when geometry causes the failure.
Assert state changes and invariants rather than calling the same helper twice.

Use the parameters documented in `games/rat-expedition/README.md`: `--evidence-dir`,
`--content-dir`, `--width`, `--height`, `--smoke-route` and `--smoke-frames`.
`scripts/stride/verify-game.ps1 -PackageZip <ZIP>` checks an extracted package from
an unrelated working directory. Record source/package manifest, backend/adapter,
logs and actual backbuffer captures. Synthetic routes are executable automation,
not OS input or manual playtests. Headless checks cannot prove focus, rendering
or Game Studio interaction; record those as unverified until exercised directly.
Link residual findings according to `AGENTS.md`; legacy C++ replay/save formats
and archived checks do not establish current Stride behavior.
