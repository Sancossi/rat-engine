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

Pending.

Included regression: [[gui-input-and-framebuffer-scale-diverge]].
