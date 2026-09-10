# Dremma scale and native runtime loading — 2026-09-10

[Task](../../vault/production/tasks/feat-dremma-game-integration.md) and
[specification](../dremma-game-integration-spec.md). This is acceptance evidence
for slice 2 only. The engine remains pinned to
`c0b9065d6e902b45d4d3a5c656318c70df53a6f3`; no Stride source change was needed.

## Resulting behavior

- Courtyard and Sluice keep their scene, geometry, portal, spawn and traversal IDs
  and topology, with authored positions and sizes baked at 2.25 times their prior
  values. Standing height is 1.8 m, crouched height 0.9 m and radius 0.45 m.
  Speed remains 3 m/s, crouch speed 1.5 m/s, climb speed 1 m/s, and gravity,
  fixed ticks, input/FSM behavior, animation timing and party spacing 0.7/1.4 m
  remain unchanged.
- The orthographic camera uses 11.25 by default with a 10.125–15.75 range. Its
  offset, aim height, edge inset and wheel world step scale by 2.25 while retaining
  the prior angle. Sprites use 60/2.25 pixels per unit and retain their feet anchor,
  atlas timing and `PointClamp` sampling.
- `NativeVisualComponent` is serialized in Authoring and editable in the normal
  solution. It binds native model resources to geometry GUIDs, can replace the
  corresponding procedural collision preview, and selects one or more mesh subsets
  by skeleton node name. The normal Courtyard contains one authored amber lantern;
  QA also exercises independent bridge deck, ironwork and post subsets.
- Windows prepares all native models for a candidate before activation. Subsets
  retain the source skeleton and shared material slots, then apply component-local
  overrides without mutating the source model. Candidate-owned content is retired
  after its representation; borrowed mesh and draw buffers are not disposed by the
  game. Nested finite TRS is preserved. Degenerate, sheared, invalid-selector and
  invalid-material candidates are rejected while the active scene stays intact.
- Presentation associates every linked model component with its geometry IDs.
  Occlusion is aggregated per model and combines with the component's authored
  enabled state, so an authored-disabled model is never enabled by an occlusion
  cycle and one visual linked to multiple geometry groups is order-independent.
  `Rat.Expedition.Core` public definitions remain free of Stride, GPU and content
  URL types.

The library source assets, IDs, FBX hashes and license/credit files from slice 1
were not changed. The only cloned scene identity correction is confined to the QA
invalid-resource fixture; its TraversalScene entity intentionally retains the
production Sluice ID while the top-level asset has a distinct ID.

## Review corrections

Independent review found and the implementation fixed these regressions before
packaging:

- a valid subset followed by an invalid selector could retain a stale generated
  preview across Undo; failure now invalidates the cache and Undo rebuilds the
  selected model;
- occlusion could overwrite an authored `Enabled=false`; tests cover repeated
  hide/show cycles for initially enabled and disabled linked models;
- matrix decomposition alone admitted shear and degeneracy; reconstruction checks
  now reject unsupported world transforms and accept valid nested TRS;
- normal and qualification Stride content builds could reuse incompatible publish
  intermediates; the package script cleans the game project's Release outputs
  before publish while preserving locked package caches and feeds;
- scaled ramp support still sampled the former 0.2 m body radius; it now uses
  `TraversalMotor.Radius`, with a 0.45 m ramp-footprint regression;
- body and mixed-support smoke observations used exact or mistimed floating-point
  predicates. The body route uses a 0.001 m observation tolerance. The mixed route
  releases horizontal input once descent begins and observes after simulation and
  occlusion update.

The final mixed observation is geometric rather than inferred from trail mode.
At frame 738 the leader is Falling at Y=2.8643193, the first companion is Falling
and visible at Y=3.5643194, and the rear feet remain on the upper support at Y=3.6.
The rear TrailPose mode is Falling because it records the arriving segment, while
`LocalOcclusion.Supports` correctly hides only that supported rear actor. Both
`bridge-cut` and `bridge-rail-cut` are active and actor visibility is
`true,true,false`. The later grounded straight segment separately retains exact
0.7/1.4 m spacing within 0.001 m.

## Packages and executed verification

All package processes used an unrelated working directory and the extracted ZIP,
not loose source JSON. Both package builds restored the locked cohort, ran 64 Core
and 20 Authoring scenarios, cleaned the game project's Release outputs and
published self-contained win-x64 binaries.

| Artifact or command | Observed result |
|---|---|
| Final normal ZIP `build/stride-game/20260910-154547-616/rat-expedition-0.1.0-win-x64.zip` | SHA-256 `13CF1AD5D0C13135826463DF02A49CD47538A2B63125DF20C4A3F1D2D43F1492`; manifest commit `d12a653dc4c3d2fc009303ce890201ec134c6b49`, dirty=false, qualification=false, pin exact |
| Final QA ZIP `build/stride-game/20260910-154109-419/rat-expedition-0.1.0-win-x64.zip` | SHA-256 `A699148B8657A5151AC3B05C37077FC843BCD7CB8FD3B6D4A4584247867003C5`; manifest commit `9a299a70e9851976b70df3e48d04935d86a44e5b`, dirty=false, qualification=true, pin exact. Later `d12a653` changes only an external verifier comment; runtime/content are identical. |
| Normal outside-checkout extraction `C:/rat-expedition-validation/slice2-normal-20260910-155028-215/result.json` | Exit 0 from owned PID 67976; unrelated working directory, 360 frames, Courtyard, camera 11.25 and 1 native visual/3 meshes. Its run reports NVIDIA GeForce RTX 4090 / Direct3D11, 3 material slots, 1 active native lease and 70 procedural buffers. Frames 30/180/360 exist; visual inspection of frame 360 confirms the amber lantern renders. |
| Final affected/remaining QA run `C:/rat-expedition-validation/20260910-154322-970/verification.json` | PASS, ten scenarios: layered 720p and 1080p, mixed locality, 20 portal legs/revision 20, renderer candidate failure, native candidate failure, native resource failure, and Courtyard/Sluice/upper-void recovery. |
| Earlier unchanged-scenario run `C:/rat-expedition-validation/20260910-150916-130/` against clean QA commit `d972d65` | GPU 720p/1080p, both camera edges, wheel clamp, both ten-milestone body routes and pause, bridge subsets, large-Y startup, four PNG/font failures, eleven compiled-native failures, and layered 720p/1080p all passed before the then-obsolete mixed assertion stopped the run. |
| `python scripts/check_vault.py`; `python -m unittest discover -s scripts/tests` | PASS; vault valid and 25 Python tests. Fixture generation produced the qualification package successfully. |
| Parent `scripts/stride/build.ps1` | PASS at `C:/5_gamedev/stride/logs/rat-foundation/20260910-154754-185/result.json`; 66.65 s, 5 warnings, 0 errors, clean exact pin. Editor: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. |

The earlier bridge-subset run reports 4 native visuals, 12 meshes, 24 shared
material slots and 1 active native lease. The final portal run completed 20 legs
and revision 20; each scene retained stable per-scene visual/mesh counts and the
active lease count remained 1 throughout. The final invalid native-resource
candidate reported selector `absent-node`, retained Courtyard at revision 0 and
retained exactly 1 visual/3 meshes/3 slots/1 lease. Renderer and native-definition
candidate failures likewise preserved the current world; all three recovery cases
returned a standing party to the correct safe layer with cleared occlusion.

## Verification limit

The standard window-state preflight remains pending. Repeated unchanged verifier
runs initially observed the game focused, then lost foreground ownership while the
unrelated `WowClassic` process (PID 68224 at diagnosis time) held the OS foreground.
No test or build process interacted with that process. Evidence includes
`C:/5_gamedev/rat-expedition-validation/20260910-145522-171/` and
`C:/5_gamedev/rat-expedition-validation/20260910-145735-659/`.

An untracked copy of the committed verifier omitted only that window-state
preflight for `C:/rat-expedition-validation/20260910-150916-130/`. After the
runtime radius and mixed-route corrections, a second untracked focused harness ran
only the ten affected and remaining scenarios recorded in the final verification
JSON. No committed assertion or game behavior was skipped or weakened. There is
therefore no claim of one complete standard-verifier PASS; the unchanged OS focus
gate must be rerun during final integration acceptance.

This evidence is from automated GPU runs on this development PC, not a clean
machine, remote CI or manual keyboard playtest. It does not accept slice 3: the
normal game still starts in Courtyard, and Dremma map construction, stairs, portals,
door clips and final complete verifier run remain for the next slice. It also does
not expand the sound/font/full-library or unbuilt-catalogue scope.
