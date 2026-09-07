---
type: task
area: Engine
status: In progress
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
