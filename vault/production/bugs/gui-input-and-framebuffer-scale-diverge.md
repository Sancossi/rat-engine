---
type: bug
area: Engine
status: Investigating
severity: Medium
sprint: Sprint 15
tags: [bug, stabilization]
---

# GUI input and framebuffer scale diverge

Origin: [[s15-acceptance]] — read-only GUI integration review during implementation.

## Evidence

FrameCoordinator simulates before current ImGui frame capture flags are available. NativeWindow polling and ImGui callbacks use separate input paths. Cursor coordinates are logical but viewport unprojection uses framebuffer dimensions; imgui_bgfx forces framebuffer scale to one. These are code findings to lock down with the approved scripted-input and UI-scale regression scenarios.

## Acceptance

Shared frame input reaches ImGui and viewport consistently; UI capture blocks gameplay input. Logical/framebuffer conversion is explicit, with automated 100/150/200 percent scale click/drag and render checks. Resolve within the already-approved stage 6 GUI work.
