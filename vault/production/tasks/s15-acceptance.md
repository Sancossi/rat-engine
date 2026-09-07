---
type: task
area: Engine
status: In progress
review: In review
task_type: Chore
sprint: Sprint 15
tags: [task, stabilization]
---

# Automated GUI and delivery

Origin: [[project-audit-followups]].
Contract: [approved implementation plan](../../../docs/superpowers/plans/2026-09-07-stabilization.md), stage 6.

A15–A16: full real-editor scripted GUI tests, paths/package, headless builds, CI/sanitizers/static checks and benchmarks.

Acceptance: stage contract met; meaningful tests passed; independent review Approved; full editor Release rebuilt at stage close.

Depends: [[s15-geometry]].

## Resolution

In progress: stage 6a build/launch/package foundations, then real GUI runner/scenarios, then CI/static analysis/benchmark. Preliminary local Clang Analyzer check passed 43/43 rat_core and rat_editor_logic translation units using clang-tidy 22.1.8, warnings as errors. Report: build/stabilization/tidy-preflight/report.json. This preflight predates delivery/GUI changes; a final run is required.

## Bugs found

Launch review caught default-directory lookup before explicit argument handling; fixed before stage-6a commit. Startup shutdown could dereference an uninitialized ImGui backend; guarded in stage 6a, real invalid-map process test remains GUI acceptance.

Included regression: [[gui-input-and-framebuffer-scale-diverge]].


Stage 6a implemented in 3c4ce34: full and fresh headless Windows builds each passed 703/703 tests; renderer-only rat_engine built, no GLFW/ImGui/audio dependencies. Header dependency proof passed after shared app extraction. Parent full Release build passed. Install and ZIP built with data/licences/runtime; installed --help passed from unrelated temporary cwd. Independent review Approved: 5 launch tests / 39 assertions passed. GUI/package resources are not yet verified; stage 6b is active.


GUI infrastructure committed in 22eb27d. Actual local software renderer report: Direct3D 11 / WARP; real Enter Edit click and ordered quick Backspace tap passed. Parent inspected captured PNG and rebuilt full Release successfully. Artifacts: build/stabilization/gui-input2/artifacts/scenario-report.json, first-edit-frame.png and first-edit-frame.json. Regular 703 tests passed. Infrastructure review Needs fixes: quick complete taps are lost by gameplay/viewport final-held-state sampling; deferred actions can settle before ImGui trickled input queue is exhausted. Real quick-action/click and multi-key-text-plus-Close/F5 regressions required. Full scenario suite remains in progress. GUI work found duplicate ImGui IDs between viewport radios and terrain buttons; fixed with distinct widget IDs.


GUI input corrections committed in d5ae2f4. Actual WARP input-order and authoring-text scenarios passed, including quick F2/F5/viewport taps and queued multi-key text before Close/F5. Parent inspected the modal capture showing complete field content. Full verification actual process exit 0: 703/703 C++ tests, 8 Python checks and vault. Parent full Release build passed. Infrastructure re-review pending; remaining GUI groups are still in progress.


Evidence correction: early GUI runs proved real rendering/input, but their WARP label recorded requested mode, not verified device. Pinned bgfx can fall back to adapter 0; actual software-device proof is withdrawn until explicit WARP context and DXGI software-adapter verification are implemented and tests rerun. Independent d5ae2f4 review confirms original input fixes, but queued clicks still combine processed buttons with raw cursor coordinates. A Unicode regression was accidentally ASCII question marks due shell encoding. Both regressions are being corrected before infrastructure approval.


Correction 68187f4 is in independent review: processed cursor positions follow ImGui event timing; native cursor events are ordered; Unicode assertions use explicit escapes. Three real GUI scenarios passed with an explicitly created WARP device verified as Microsoft Basic Render Driver with DXGI_ADAPTER_FLAG_SOFTWARE. Evidence: build/stabilization/verified-warp/{infrastructure,input-order,authoring-text}/artifacts. Parent inspected cyrillic-field.png and full Release rebuild passed. Full GUI scenario and scale acceptance remains in progress.


Infrastructure correction 68187f4 independently Approved: all three scenarios passed on verified WARP; reviewer checked cursor ordering, Unicode capture and device ownership through shutdown. Independent artifacts: build/stabilization/review-warp-ch6tjfh6/. Full scenario/scale coverage is now being implemented and its review remains pending.


GUI workflow slice 554fe02 is in independent review. Twenty-one new scenarios passed: twelve destructive-action modal combinations, failed-save retry, map and slot backup restore, malformed startup/load, two real restart processes, three creation controls/duplicate error, and history/voxel no-op/abort/undo/redo. Actual production editor malformed-map launch exited 1 cleanly. Full verification: 703 C++ tests, eight Python checks, vault; parent full Release rebuild passed. Logs: build/stabilization/gui-guards-verify.log and gui-native-malformed/report.json. Real GUI exposed Ctrl+Z blocked after button navigation focus; corrected while retaining text-input ownership. Graph, ramp, bridge, unsupported repair, scale and packaged GUI groups remain in progress.


Independent review Approved 554fe02: all 21 new scenarios passed again; artifacts build/stabilization/review-guards-sh5ixh8v/. No blocking findings. Remaining suite must prove modal pause in an advancing Play session, since the initial guard matrix starts in Edit. Whole stage remains in progress.


GUI geometry/graph/repair slice ddc8d9c is in independent review. Eleven new WARP scenarios passed: four side-ramp directions, three bridge variants, graph gestures/layout history, advancing-Play modal pause, EventTouch repair and cross-map transfer repair. Full verification 703 C++ tests, eight Python checks and vault passed (gui-geometry-graph-verify.log); parent full Release rebuild passed. GUI found graph pins shifting when connection status appeared and duplicate undo handling; both corrected. Artifacts: build/stabilization/gui-geometry/, gui-graph/, gui-unsupported/. Scale/framebuffer and aggregate/package scenarios remain in progress.
