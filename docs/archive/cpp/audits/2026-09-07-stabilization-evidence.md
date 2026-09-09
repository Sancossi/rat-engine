# Stabilization evidence index

The [approved plan](../superpowers/plans/2026-09-07-stabilization.md) is implemented.
The [original audit](2026-09-07-project-review.md) remains a historical baseline.
All implementation slices received independent approval after any review fixes.
Local Windows acceptance is complete. Hosted CI, Linux, ASan/UBSan and the hosted
source-absent package job are configured but have not been executed or observed.

| Audit | Implemented behavior and regression evidence | Review |
| --- | --- | --- |
| A01 | Shared Save/Discard/Cancel controller, failed-save retry, complete 4-by-3 GUI guard matrix, advancing-Play pause and input reset | Approved |
| A02 | Atomic checked replacement and one backup, failure injection; GUI pinned map backup and save-slot restore | Approved |
| A03 | Strict save v2, valid legacy reader, transactional load and apply; game state/session regressions | Approved |
| A04 | Finite/range validation before conversion and derived geometry, strict graph parameters; map loader/document regressions | Approved |
| A05 | Shared unique allocator, duplicate rename rejection; all three real GUI creation controls and two distinct restart processes | Approved |
| A06 | Cross-map transfer rejected with precise paths and runtime guards; real UI repair followed by Apply/Play | Approved |
| A07 | Complete canonical authoritative replay state checksum and semantic map fingerprint; replay regressions | Approved |
| A08 | Strict replay v2 header/ticks/config/freshness, zero checksum compared; linear append cost retained | Approved |
| A09 | Canonical authored clean snapshot, changed/no-op result, history and layout persistence; GUI undo/redo/stroke/layout cases | Approved |
| A10 | EventTouch explicitly rejected, disabled UI choice, authored draft repairable; real repair/Apply/Play scenario | Approved |
| A11 | Filename-based vault links and executable link/status validation | Approved |
| A12 | Explicit zero/one current sprint, board validation and reconciled included follow-ups | Approved |
| A13 | Actual targets, feature boundaries and implemented/partial/planned documentation | Approved |
| A14 | Root AGENTS, shared build/vault/runtime skills, Cursor references and reproducible build wrapper | Approved |
| A15 | Negative regressions, full 40 real GUI manifest, CI/sanitizer/static-analysis configuration with fatal failures | Approved; remote/Linux execution unobserved |
| A16 | Independent headless/renderer gates, UTF-8 launch roots, install/ZIP, installed full 40 GUI, benchmark and metadata | Approved; hosted source-absent job unobserved |

Included Sprint 14 follow-ups are complete: empty voxel removal preserves redo;
all four side-ramp yaw placements, nondegenerate outward wedge faces and exact
fences; occupancy round-trip/boundary validation; thin bridge support/underpass
and filled-solid negative. Graph self/source/empty/rejected releases clear the
active drag, with actual UI regression coverage. Additional findings corrected
include explicit-empty-graph migration, slab/ramp height mismatch, numeric
intermediate overflow, localized MSVC header dependencies, trickled text/cursor
ordering, graph origin shift, duplicate undo and graph display scaling.

## Final local verification

| Check | Observed result | Evidence under `build/stabilization/` unless noted |
| --- | --- | --- |
| Full Windows Release | 707/707 non-GUI CTest entries passed | `final-release-verify.log` |
| Fresh headless | 707/707 passed, no graphics/audio FetchContent | `final-headless-{configure,build,tests}.log`; `build/final-headless` |
| Fresh gui-release | 708/708 CTest entries, including full 40 GUI aggregate | `final-gui-verify.log` |
| Installed ZIP | Full 40 passed in 60.64s; Unicode/spaced paths, real glyph/audio loading, package/cwd unchanged | `final-package-artifacts/gui-run-9tgdxxgk/gui-acceptance-report.json` |
| Independent full GUI | Full 40 passed in 60.10s on verified WARP | `review-final-artifacts/gui-run-1_fw67v0/gui-acceptance-report.json` |
| UI/framebuffer scale | 100/150/200 UI scales, scripted 1.25 framebuffer ratio; PNG 1700x975/2500x1425/3300x1875 | Independent full GUI report and captures |
| GUI-only install | Infrastructure passed without editor executable | `gui-only-artifacts/gui-run-fjriumqk/gui-acceptance-report.json` |
| Clang Analyzer | 45/45 own core/editor-logic translation units passed, warnings fatal, clang-tidy 22.1.8 | `tidy-final/report.json` |
| Repository/tooling | Python 10/10, vault, actionlint 1.7.12 passed | `actionlint.log` and verification logs |
| Benchmark | Four maps, 1,000 warmup and 10,000 measured ticks each, bake/apply/load and retained-memory phases | `final-benchmark.json`; committed baseline below |

Scale tests exercise scripted logical/framebuffer dimensions through the native
input/render path; they do not change Windows DPI settings. WARP is explicitly
created and its actual DXGI software adapter is verified. Initial infrastructure
runs only requested software mode; later verified runs supersede those labels.
Memory estimates cover known retained objects/capacities, excluding allocator
and opaque callable overhead; they are not process RSS or serialized map sizes.

## Artifacts and review trail

- Editor: `build/dev-release/apps/editor/rat-editor.exe`.
- GUI-enabled editor: `build/gui-release/apps/editor/rat-editor.exe`.
- Final ZIP: `build/final-packages/rat-engine-0.1.0-Windows-AMD64.zip`.
- [GUI contract and full manifest](../gui-automation.md).
- [Storage ADR](../adr-storage-v2.md), [replay ADR](../adr-replay-v2.md).
- [Benchmark methodology](../benchmark.md) and [committed baseline](../benchmarks/2026-09-07-windows-release.json), measured implementation `6694640`.
- [CI and analysis contract](../continuous-integration.md).

Approved implementation checkpoints: foundation `c11288b`/`83746e7`, storage
`ebe7759`/`c90638f`, authoring `160a376`, runtime `eed6158`/`4eaeaf6`, geometry
`2a44876`, launch/package `3c4ce34`, GUI input `68187f4`, GUI workflows `554fe02`,
GUI geometry/graph/repair `ddc8d9c`, full local GUI `4efcb52`, memory/benchmark
`6694640`, baseline `f82f5df`, CI/final tooling `b78b614`, final README `57726e7`.
Detailed review and correction history remains in the Sprint 15 cards.
