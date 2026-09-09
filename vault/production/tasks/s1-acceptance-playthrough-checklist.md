---
type: task
area: Production
status: Done
task_type: Chore
sprint: Sprint 1
roadmap: Grey-box + Event runtime v1
due:
tags: [task]
notion_id: 3cdf3827-36cc-81eb-a4d8-c20263a2b6f2
---

# S1: Acceptance playthrough checklist

## DoD

- Checklist in wiki passed
- Known issues listed
- Demo path recorded (notes)

## Status (2026-08-31)

Passed against branch `cursor/glfw-imgui-editor-shell`. Full write-up: repo `docs/archive/cpp/sprint1-acceptance.md`.

## Demo path

1. Launch `rat-editor` → dismiss autorun with E/Space
2. Talk to foreman (cyan, ~-2,2) → accept quest
3. Walk around crates → loot scrap (~6,0) → rusty_cog
4. Return to foreman → turn-in (quest complete)

Automated: `ctest` case *grey_yard cog quest end-to-end*.

## Checklist

- [x] Ortho grey-box + player visible
- [x] Camera-relative move + snap centers
- [x] JSON events + dialog/interact
- [x] Quest via switches/items
- [x] Parallel limits / ctest green

## Known issues

- Grey pillars only (no meshes)
- Highest-page RM matching — author carefully
- No multi-map transfer / file save UI yet
- Parallel over-budget warns in runtime list, not ImGui toast
