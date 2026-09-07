---
type: bug
area: Engine
status: Fixed
review: Approved
severity: High
sprint: Sprint 15
tags: [bug, stabilization]
---

# MSVC localized include dependencies missed

Origin: [[s15-authoring]]; follow-up to [[s15-foundation]].

## Repro

Change EditorDocument.hpp after an initial dev-release build with localized MSVC. Ninja leaves event_graph_edit_test.obj unchanged because detected showIncludes prefix does not match actual compiler output. Test assertions pass but stale class layout produces process exit 0xc0000409.

## Expected

A header change rebuilds every dependent own object in the ordinary verification workflow, with the installed compiler locale. No clean build is required for correctness.

## Actual

CMake/Ninja include-prefix encoding differs from compiler output despite VSLANG=1033. A successful incremental build can contain stale objects.

## Resolution

Implemented in 2d83e7a. verify.ps1 -CheckHeaderDependencies passed: header timestamp change rebuilt 11 dependent translation units and checked three object timestamps. Full verification with current authoring slice: 663/663 C++ tests, 8 Python checks, vault passed. Parent unchanged full Release build returned no work, exit 0. Independent review Approved: Ninja dependency records confirmed for editor logic and affected graph tests; independent dry run also no-op. Evidence: build/stabilization/include-prefix-header-verify.log and include-prefix-deps.log.

## Bugs found

None. Standard MSVC/Ninja toolchain verified; custom compiler wrappers are outside this verification.
