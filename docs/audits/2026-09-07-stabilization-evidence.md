# Stabilization evidence index

Implementation of the [approved plan](../superpowers/plans/2026-09-07-stabilization.md).
The [original audit](2026-09-07-project-review.md) describes the baseline and remains historical.
This index distinguishes completed implementation from pending desktop/delivery acceptance.

| Audit | Implementation and regression evidence | Independent review / remaining acceptance |
| --- | --- | --- |
| A01 | `160a376`: shared destructive-action controller, transactional Save/Discard/Cancel; `tests/editor_document_test.cpp` | Authoring approved; complete real GUI guard matrix pending |
| A02 | `ebe7759`, `c90638f`: checked atomic replacement, one backup, failure injection; `tests/file_store_test.cpp` | Storage approved; GUI explicit restore cases pending |
| A03 | Strict save v2 and valid legacy reader; `tests/game_state_test.cpp`, `tests/simulation_session_test.cpp`; [format ADR](../adr-storage-v2.md) | Storage approved |
| A04 | Checked map numbers and derived geometry; `tests/map_loader_test.cpp`, `tests/map_document_test.cpp` | Storage approved after malformed-parameter and overflow corrections |
| A05 | Common unique allocator, transactional duplicate rename; `tests/event_edit_test.cpp`, `tests/editor_document_test.cpp` | Authoring approved; GUI creation paths and two-process restart pending |
| A06 | `eed6158`: unsupported cross-map transfer rejected before apply and defended in runtime; map/session/event tests | Runtime approved; GUI repair scenario pending |
| A07 | Complete canonical authoritative replay checksum; `tests/replay_test.cpp`; [replay ADR](../adr-replay-v2.md) | Runtime approved |
| A08 | Strict replay v2 header/ticks, freshness and semantic map identity; `4eaeaf6` preserves linear recording cost | Runtime approved; replay tests and append scaling measured |
| A09 | Canonical authored clean snapshot, changed/no-op distinction, graph layout history; editor document/history tests | Authoring approved; full GUI undo/layout cases pending |
| A10 | EventTouch rejected with repairable authored document and disabled UI choice; map/runtime tests | Runtime approved; real GUI repair/disabled-choice scenario pending |
| A11 | `c11288b`: filename-based vault links and `scripts/check_vault.py` | Foundation approved; vault checks pass |
| A12 | Explicit zero/one current sprint, board validation and Sprint 15 queue | Foundation approved; final sprint closure awaits acceptance |
| A13 | `83746e7`: actual architecture and feature boundaries documented | Foundation approved; delivery docs will reflect final acceptance |
| A14 | Root AGENTS, shared project skills, Cursor references, build wrapper | Foundation approved; three project skills validated |
| A15 | Negative regression suite; real GUI infrastructure `68187f4` on verified WARP | Infrastructure approved; full GUI suite, CI/sanitizers and final static analysis pending |
| A16 | `3c4ce34`: independent headless/renderer gates, launch roots, install/ZIP; launch tests | Launch foundations approved; packaged GUI and final delivery acceptance pending |

Included Sprint 14 follow-ups: empty voxel no-op is covered by the authoring
changes; `2a44876` covers all-yaw side placement and wedge faces, occupancy
round-trip/boundary validation, thin bridge support and a filled-solid negative,
and graph wire-release cancellation. Independent geometry review passed six
focused tests with 1,314 assertions. Desktop gesture coverage remains part of
stage 6. Review also found and fixed slab/ramp height mismatch and numeric
overflow. `2d83e7a` fixes localized MSVC header dependency discovery, verified by
touching a header and observing recompilation.

Latest completed full verification at the GUI infrastructure checkpoint:
703 C++ tests, eight Python checks, vault validation and full Release build.
Three real GUI scenarios (`infrastructure`, `input-order`, `authoring-text`)
passed on an explicitly created WARP device whose DXGI software flag was checked.
Parent and independent reviewer inspected Cyrillic captures. Evidence:
`build/stabilization/verified-warp/` and
`build/stabilization/review-warp-ch6tjfh6/`. Earlier renderer labels described a
request only; these later runs establish actual software-device evidence.

Fresh Windows headless build passed 703 tests. Renderer-only target built without
GLFW/ImGui/audio. Install and ZIP creation plus installed CLI help passed; this
alone does not prove packaged graphical resources. Preliminary Clang Analyzer
passed 43 own core/editor-logic translation units; final analysis is pending.
Linux, sanitizers and remote GitHub Actions have not been executed locally or
observed remotely. They must not be reported as passed from workflow definitions.
