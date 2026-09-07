---
type: bug
area: Engine
status: Fixed
review: Approved
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


Infrastructure review 22eb27d: ordered events reached ImGui but final held-state sampling lost quick gameplay/viewport taps. ImGui trickling also retained text past deferred Close/F5. These in-scope input fixes and real-widget regressions are active in [[s15-acceptance]].


Re-review d5ae2f4: original edge/text-queue fixes pass real GUI checks. Remaining cursor timeline mismatch: queued clicks at A must not use later raw cursor B. Unicode fixture encoding and actual software-device verification also require correction in the same acceptance stage.


Correction 68187f4 is in independent review: processed cursor positions follow ImGui event timing; native cursor events are ordered; Unicode assertions use explicit escapes. Three real GUI scenarios passed with an explicitly created WARP device verified as Microsoft Basic Render Driver with DXGI_ADAPTER_FLAG_SOFTWARE. Evidence: build/stabilization/verified-warp/{infrastructure,input-order,authoring-text}/artifacts. Parent inspected cyrillic-field.png and full Release rebuild passed. Full GUI scenario and scale acceptance remains in progress.


Infrastructure correction 68187f4 independently Approved: all three scenarios passed on verified WARP; reviewer checked cursor ordering, Unicode capture and device ownership through shutdown. Independent artifacts: build/stabilization/review-warp-ch6tjfh6/. Full scenario/scale coverage is now being implemented and its review remains pending.


## Resolution

Fixed across 22eb27d, d5ae2f4, 68187f4 and 4efcb52. Shared ordered input and processed positions feed GUI/gameplay consistently; pending destructive actions drain text input and pause Play. Logical/framebuffer transforms and graph display scaling preserve actual hit positions and authored layout. Independent full Windows WARP suite passed40/40, including100/150/200 UI scales, native resize and scripted1.25 framebuffer ratio; captures match1700x975/2500x1425/3300x1875. Report build/stabilization/review-final-artifacts/gui-run-1_fw67v0/gui-acceptance-report.json. Parent full Release build passed. This tests scripted framebuffer dimensions, not changing Windows DPI settings.

## Bugs found

Quick-tap loss, trickled cursor/text timing, graph origin shift and duplicate undo were found and fixed within acceptance. No unresolved input/scale findings.
