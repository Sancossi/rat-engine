# Dremma game integration — approved 2026-09-10

User requested the existing CanalCity graphics in the current game, chose a NEW
playable location and revisited scale. Start the normal game in Dremma, with a
two-way portal to Courtyard and the existing Courtyard/Sluice route intact.
This explicit plan authorizes the scoped A1 resource/runtime dependency work here;
it does not close the entire A1 library or the unfinished CanalCity catalogue.

## Three sequential acceptance slices

1. **Library**: merge CanalCity history c74c6e77e54492256f818e0963c3ec423e8c9c75
   into the current accepted project, retain all 97 native assets, 12 model/prefab
   pairs, 45 textures, 11 materials, two door clips, source FBX/Blender/credits and
   prior evidence. Authoring owns Assets/CanalCity and Resources/CanalCity. Verify
   actual current-editor open/edit/Undo/Redo/Save/fresh reopen and native reimport
   on OWN COPIES of one FBX and one PNG without changing original library IDs.
   No general MCP import/reimport tool expansion. Isolated preview is supplementary,
   not proof of normal game integration. Review/full Release/main before slice 2.
2. **Scale and runtime loading**: keep Core free of Stride/GPU dependencies. Load
   authored visuals into Windows scene presentation with explicit resource ownership,
   fallible preparation before scene activation and retirement afterwards. Borrowed
   model buffers must not be manually disposed. Authoring visual bindings use native
   geometry IDs and identify replacement of procedural geometry and occlusion
   membership; do not render both visible collision cubes and replacement art.
   Change physical standing/crouched height to 1.8/0.9 m and radius .45 m. Retain
   Speed=3, CrouchSpeed=1.5, ClimbSpeed=1, gravity, FSM, fixed ticks, input semantics,
   animation timing and party spacing .7/1.4. User correction: a bigger city is NOT
   a reason to change hero behavior. Sprite dimensions and camera world framing
   scale by 2.25; camera default11.25, min10.125/max15.75, same angle and feet anchor.
   Courtyard/Sluice authored geometry/points migrate by2.25, preserving IDs/topology.
   Bake geometry positions/sizes; no scaled/rotated gameplay ancestor transforms.
   Longer physical routes take longer at unchanged speed. No global source-art
   scale: new CanalCity FBX is already in metres. Review/full Release/main before3.
3. **Playable Dremma**: compact start plaza -> waterfront/lanterns/railings -> stairs
   and upper bridge path -> return/portal to Courtyard, plus lower path under bridge.
   Use native-scale finished assets; connect height datums using authored platforms
   and supported ramps, not new step locomotion. Colliders are separate finite
   gameplay volumes preserving actual openings/support. Separate deck, piers,
   railings/attached decoration for occlusion; support under player remains visible,
   unrelated models stay visible. Four scale figures are static references, not NPCs.
   Door outside required route; retain/test both clips, no door interaction or dynamic
   collision feature. New native gameplay scene separate from PreviewScene.

## Acceptance and limits

Each slice: one writing implementer and independent read-only reviewer, exact
evidence, vault/projection checks and full parent Release editor build; publish
accepted commits to main and verify remote SHA. Stride pin/game package content
hashes unchanged unless a concrete separately reported dependency demands a patch.
Preserve existing source worktrees and unrelated edits. Source art generation must
not overwrite manually edited gameplay scenes.

Final: test .45/1.8 and .9 clearances, exact retained speeds, camera framing720/1080,
FSM/crouch/recovery/pause, party trailing, bridge upper/lower support and occlusion,
stairs and10 portal cycles. Broken resource/candidate preparation preserves active
scene; repeated transitions do not accumulate resources. Re-run meaningful old
Core/authoring/runtime gates with updated physical dimensions and route durations,
never weaken failure assertions merely to pass. Build committed normal+QA ZIP,
verify extracted files outside checkout and dependency/notice closure. API controls
editor; actual images prove appearance only. No manual GUI/clean-machine/remote CI
claim from automation. A1 sound/font/full-library and123 unbuilt catalogue models
remain outside this request.
