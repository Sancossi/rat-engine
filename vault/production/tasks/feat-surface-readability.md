---
type: task
area: Engine
status: In progress
review: Pending
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

In progress. Standalone feature; no new sprint or unrelated queue activated.

## Bugs found

None yet.
