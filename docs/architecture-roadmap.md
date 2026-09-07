# Architecture roadmap

Updated 2026-09-07 against the current source tree. Sprint 7 established most
runtime interfaces; Sprint 15 stabilizes their contracts. Historical sprint cards
retain their original scope and acceptance evidence.

## Implemented and partial foundations

| Area | Current behavior | Remaining boundary |
| --- | --- | --- |
| Simulation | Shared SimulationSession fixed-tick player/state/event evolution | Replay completeness/validation in Sprint 15 |
| Authoring | MapDocument compiles to RuntimeMap; editor document and undo/redo exist | Dirty semantics, no-op history and save safety in Sprint 15 |
| Events | Play executes graph nodes/edges directly; legacy commands migrate on load; command structs supply node effects | Cross-map loading and EventTouch unsupported; reject explicitly in Sprint 15 |
| Presentation | RenderWorld/packets and bgfx adapter exist | Independent render-library split planned |
| Editor | Panels, document and coordinator extracted | Shared scripted input, modal acceptance and DPI checks in Sprint 15 |
| Assets | Stable AssetId, registry, states and MemoryAssetLoader | Disk mesh/texture import and production GPU loading planned |
| Entities | Generational EntityId and ComponentStore scaffolding | Player/events retain existing types; no ECS migration |
| Platform | File/log/input adapters and queued miniaudio playback exist | Portable paths, packages and fault-safe replacement in Sprint 15 |
| Instrumentation | Timing/debug snapshots and render counters exist | Reproducible performance baseline in Sprint 15 |

Actual targets: `rat_core`, `rat_engine`, `rat_editor_logic`, `rat-editor`, `rat_tests`.
Runtime, authoring, assets and entity scaffolding share `rat_core`; the following
graph is future decomposition, not an implemented set of targets.

## Future library separation

```text
apps/editor -> rat_authoring / rat_runtime / rat_render / platform adapters
rat_authoring -> rat_runtime / rat_core
rat_runtime -> rat_core
rat_render -> rat_assets / rat_core / bgfx adapter
rat_assets -> rat_core / explicit loader adapters
```

Introduce targets only as a separately scoped refactor preserving public behavior
and link-isolation tests. Acceptance: runtime/authoring tests compile without
platform/graphics headers; adapters expose ownership; no cycles, locator or singleton.
Do not migrate gameplay into ECS or introduce RHI/job systems as part of this split.

## Current implementation order

The [Sprint 15 contract](superpowers/plans/2026-09-07-stabilization.md) is the active queue:
workflow/build foundation, safe storage, authoring lifecycle, runtime/replay,
geometry/graph regressions, then GUI/CI/package acceptance. Format/API changes
need ADRs and negative tests; docs must state actual limits while stages remain open.

After stabilization the art pipeline is a separate feature. Acceptance must include
real mesh/pixel-texture import, stable IDs with useful failure/fallback diagnostics,
GPU resource release at frame boundaries, and an ortho scene demonstrating accepted
palette/texel-density rules. Registry unit tests alone do not establish this vertical
slice. Segmented procedural characters and field physics remain separate work.

## Performance before optimization

Capture grey_yard and synthetic-map tick p50/p95/p99, bake/apply time, undo memory,
and machine/build/configuration metadata. Keep repeatable scenarios with the report.
Introduce broadphase, pooling, SoA or scheduling only after a measured bottleneck
with comparable before/after results. No arbitrary blocking timing threshold on
shared CI hardware before a useful baseline exists.

Principles: fixed-step simulation, immutable compiled runtime data, explicit adapter
ownership, and no graphics dependencies in gameplay interfaces.
