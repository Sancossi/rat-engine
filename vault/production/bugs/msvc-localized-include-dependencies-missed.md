---
type: bug
area: Engine
status: Investigating
review: Pending
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

Pending. Acceptance includes a controlled header-change dependency rebuild and full verification, plus independent review.

## Bugs found

Pending.
