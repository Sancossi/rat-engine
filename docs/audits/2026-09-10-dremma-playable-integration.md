# Dremma playable integration — slice 3

Date: 2026-09-10. Implements the final slice of the
[approved integration plan](../dremma-game-integration-spec.md).
Acceptance is still pending the final QA and OS window-state results.
This report does not yet close the integration card.

## Resulting game and authoring behavior

The ordinary game starts in the separately authored `Dremma` scene. Its portal
connects both ways to Courtyard; the existing Courtyard/Sluice connection and its
compiled portal ordering remain intact. The new scene has 61 entities and asset ID
`d740bf16-9a5c-5844-9eb3-6a9fdbef9c7f`. Open
`games/rat-expedition/Rat.Expedition.sln` in Game Studio, select the Authoring
package's Assets folder, and open Dremma. The imported library remains under
Assets/CanalCity and Resources/CanalCity.

Native visuals keep their original metre scale. Four static figures show
1.2/1.8/2.8/4 m reference heights. The playable body is 1.8 m standing, 0.9 m
crouched, radius 0.45 m. Speeds 3/1.5/1, gravity, FSM, fixed ticks, input,
animation timing and party spacing 0.7/1.4 are unchanged. Camera framing remains
11.25 by default with limits 10.125–15.75, as accepted in slice 2.

The bridge has a separate upper deck, finite piers, and 26 finite arch segments
that preserve its curved lower opening. Authored ramps connect the medium stairs
to the 3.1 m deck; solid stair mass and four sloping rail volumes block side
entry. Geometry and native art are linked by GUID, including multiple geometry
IDs per native visual. Local occlusion hides the relevant bridge parts without
removing the support under the party or unrelated models. The door is static,
outside the required route; its two clips are qualified separately.

Presentation prepares native resources before committing a scene transition.
Successful replacement retires the previous lease, and failure retains the
active scene. Shared model draw buffers are borrowed, never manually disposed.
Runtime shutdown now reports and verifies zero active native leases. There is no
editable runtime JSON shadow of the authored scene.

The implementation is `fb36d6c`, with authoring-preview corrections in `50aba40`
and `262176c`. The latter received independent code approval. Review found that
one invalid visual binding could suppress unrelated geometry, then that early
validation could restore an obsolete cached model after source replacement.
Both paths have regression coverage; invalid metadata remains a runtime error.

## Editor and source evidence

`afce5cd` corrected the production-scene qualification to save a dirty change,
then inspect it in a fresh editor before restoring the original value.
`build/mcp/dremma-scene-20260910-164716-855/` passed: wall Z13→12.9, Undo/Redo,
dirty Save, fresh reopen at 12.9, restoration to13 and Save. Explicit XYZ and
stable scene/entity/native-reference IDs were checked through the existing19
MCP tools. Independent review confirmed all61 entities, component fields,
transforms, references and RootParts ordering were semantically unchanged by
Game Studio's canonical YAML serialization. This is actual editor/API evidence,
not a headless substitute for opening the scene.

The initial clean-save qualification at164114 is superseded by the dirty-save
run. Review subsequently required guarded restoration when qualification fails
after Save. Fix `8361779630ddf1541e0679325bc27527fe93a44c` received independent
approval: the expected hash of the single owned edit is computed before launch,
so unrelated changed bytes are refused. A controlled client failure immediately
after native Save, before client/qualification completion, restored the original
SHA in `build/mcp/dremma-scene-20260910-170508-621/cleanup.json`. The complete
normal sequence passed again at `build/mcp/dremma-scene-20260910-170559-913/`:
fresh Z12.9, stable61 entity IDs/14 native references, restore/Save Z13, and cleanup
confirming the original SHA. No invocation temporary scene files remain.

The isolated native preview at
`build/canal-city/dremma-door-20260910-164416-749/native-preview.json` passed on
Direct3D11: 12 models, 12 prefabs and two clips. Open samples are approximately
0/50/100 degrees; close samples100/50/0. The harness checks fixed-frame and
leaf/hinge bindings. This does not introduce game door interaction or dynamic
collision.

Parent SHA-256 comparison of all170 Assets/CanalCity and Resources/CanalCity
files against source worktree commit `c74c6e77e54492256f818e0963c3ec423e8c9c75`
found no differences. The source worktree and Stride checkout remain clean;
the user's three pre-existing working-tree changes in the original rat-engine
directory were preserved.

## Executed verification

| Evidence | Observed result |
| --- | --- |
| `build/dremma-stair-rails-1` | Targeted GPU route completed at953 frames: central lower passage, standing shoulder block/crouched clearance, solid stair side block, low/mid stair support, rail block, both bridge ends and return below. Shutdown native leases0. |
| `build/dremma-portals-1` | 20 portal legs and world revision20; Dremma returns with14 native visuals/97 meshes/79 material slots and one active lease. |
| `build/dremma-resource-failure-1` | Invalid candidate selector `absent-node` leaves Dremma active at revision0 with unchanged14/97/79 and one lease. |
| Normal ZIP `build/stride-game/20260910-165143-941/rat-expedition-0.1.0-win-x64.zip` | SHA-256 `8836C75F6802EDACE210EE8A2C3F1F5ABF5C6DE0307B3C566D62C20A521F4C20`; manifest commit `fa83a149b2b7caff14f0f2d7d13b60468f4126b6`, dirty=false, qualification=false. Core64/64 and Authoring21/21 passed. |
| Parent normal-package run `C:/rat-expedition-validation/dremma-normal-20260910-165418-114/result.json` | Extracted outside checkout and launched with unrelated working directory. Owned PID41192 exited0 after360 frames; Dremma14/97/79, camera11.25, shutdown leases0. Actual frame180 inspected: bridge, library objects and the sprite party render together. |
| Normal package notices | Project/Stride/third-party notices, Content credits, font OFL and package inventory with145 entries present. |
| Parent repository checks | Vault valid;25 Python tests passed. |
| Parent full Release editor build | PASS, `C:/5_gamedev/stride/logs/rat-foundation/20260910-170329-337/result.json`;68.21 seconds,5 warnings,0 errors. Exact unchanged pin `c0b9065d6e902b45d4d3a5c656318c70df53a6f3`. Editor: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. |
| Final QA ZIP `build/stride-game/20260910-170756-029/rat-expedition-0.1.0-win-x64.zip` | SHA-256 `A0B2EFFDBE0F9E3E3746064437AE2555F8B4CADE5DA6E62642069EA535FD95E9`; manifest commit `8a1be552b2326fa94348d34850e510f25ba88131`, dirty=false, qualification=true, exact engine pin. Core64/64 and Authoring21/21 passed. |

Final verifier, projection check and publication evidence will be recorded here
before acceptance. The normal package
is a clean committed game build; later harness/documentation-only changes do not
alter its runtime/content.

## Scope and verification limits

The prior slice's OS focused/unfocused/hidden/minimized gate remains mandatory
for final acceptance. Earlier attempts lost foreground to an unrelated user
application; those failures were not called passes and no unrelated process was
modified. The final standard verifier must retain its state/timing assertions.

The final QA attempt at
`C:/5_gamedev/rat-expedition-validation/20260910-170923-393/` stopped at the focused
preflight: every observed frame30–240 was visible/non-iconic but lacked OS
foreground ownership. Unlike an earlier run, this trace does not prove focus was
first obtained and then lost. The helper discarded the activation call's return
value, so OS denial and setup failure need separate diagnosis. The owned process
exited and reported zero shutdown leases; this attempt is not a verifier PASS.

Evidence is from automated editor/API and GPU runs on this development PC, not
remote CI, a clean machine or manual keyboard playtesting. Source reimport was
qualified in [slice1](2026-09-10-dremma-library-integration.md); resource ownership
and scale regressions were qualified in
[slice2](2026-09-10-dremma-scale-native-runtime.md).
The remaining A1 sound/font/full-library work,123 unbuilt catalogue models and
broader P1 manual acceptance are outside this integration and remain open.
