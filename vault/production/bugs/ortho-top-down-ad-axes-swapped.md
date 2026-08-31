---
type: bug
area: Engine
status: Fixed
severity: Medium
tags: [bug]
notion_id: 3cdf3827-36cc-8118-8135-d31a5677cc4f
---

# Ortho top-down: A/D movement axes swapped

**Area:** Engine / player move

**Repro:** Switch to ortho top-down, press A and D.

**Expected:** A moves left on screen, D moves right.

**Actual:** A/D directions swapped.

Likely `camera_relative_move` top-down branch: `axis_x = screen_x` should be negated.

Source: chat 2026-08-31.
