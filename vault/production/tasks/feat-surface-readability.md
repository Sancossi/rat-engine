---
type: task
area: Engine
status: In progress
review: Needs fixes
task_type: Feature
sprint:
due:
tags: [task, rendering]
---

# Readable terrain and voxel surfaces

Origin: user requested implementation of the agreed surface material and silhouette proposal after Sprint 15.

Intent: distinguish tops, vertical faces, ramps, height changes and bridge undersides in the greybox editor.

Acceptance:

- Actual surface material uses fixed directional light plus ambient fill, with face normals; top surfaces light, sides differentiated, undersides dark and ramps shaded by slope.
- Dark shape/height-break contours, excluding triangle diagonals and coplanar internal voxel seams; quiet grid. Stable screen-space line weight where practical.
- Subtle height cue and explicit step outlines keep top-down readable; tilted camera exposes actual vertical faces. Preserve gameplay geometry, authoring data, selection and indoor reveal behavior.
- Compact fixture includes steps, isolated voxel, four ramps and thin bridge/underpass. Capture matching before/after real renderer images in top-down and tilted views; inspect results.
- Meaningful mesh/edge regressions, full Release and real GUI checks; portable shader build/package path, headless isolation retained. No renderer rewrite or PBR/shadow-system expansion.
- Independent review Approved, full Release editor rebuilt at closure.

## Resolution

Implemented in 72c51d7; baseline fixture/captures in 612d517. Independent review in progress. Release and headless each passed 713 tests; Python 10 and vault passed. Full 43 GUI scenarios passed on WARP; installed package subset of infrastructure plus three surface scenarios passed with package/cwd unchanged. Parent inspected matching top/tilted before/after captures and confirmed readable shapes. Evidence: build/surface-readability/final-artifacts/gui-run-qvjzd_at/; build/dev-release/gui-artifacts/gui-run-3gu0jmti/; build/surface-readability/package-artifacts/gui-run-_s2kvmih/. Standalone feature; no new sprint activated.

## Bugs found

Lighting exposed inward legacy slab/solid and inconsistent terrain-wall normals; corrected with render-only outward hints, preserving geometry/collision. Pathological overlapping face partitioning is bounded, retaining original fill and omitting uncertain contours. See docs/surface-readability.md.


Review 72c51d7 Needs fixes: contour interval extraction scans all segments per interval, allowing quadratic work on a long flat strip despite rectangle partition caps. Correcting with bounded contour processing and an adversarial strip regression. Independent six focused tests/1120 assertions passed; shader/visual results remain valid.
