---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 15
tags: [bug, stabilization]
---

# Slab ramp height mismatch removes fence

Origin: [[s15-geometry]], [[chore-ramp-voxel-review-polish]].

## Repro

Place slab top at 2.2 beside an east-facing occupancy ramp ending at 2.0. The west slab fence disappears and contact is not blocked. Aligned slab top 2.0 correctly permits the connection; removing the ramp or reversing its yaw correctly retains the fence at 2.2.

Isolated executable evidence: build/stabilization/slab-fence-probe/probe.cpp, result.txt and library-sha256.txt. Existing rat_core library copied for the probe; no shared build or source edits.

## Expected

Omit only the intended high-face connection when world heights actually align; retain fence for mismatched heights, yaw, layer or adjacency.

## Actual

append_slab_side_fences rounds top_y / tile_size to derive the layer, accepting 2.2 as 2.0 without comparing actual heights.

## Resolution

Pending; included in the approved stage-5 high-face connection contract.

## Bugs found

Pending.


## Resolution

Fixed in 2a44876; independent review Approved. Full verification 698/698 C++ tests passed. See [[s15-geometry]]; actual GUI gesture evidence remains [[s15-acceptance]].

## Bugs found

No unresolved implementation findings.
