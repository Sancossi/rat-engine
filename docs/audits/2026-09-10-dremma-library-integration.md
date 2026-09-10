# Dremma library integration — slice 1 — 2026-09-10

This is the first acceptance slice of the [approved Dremma integration](../dremma-game-integration-spec.md).
The CanalCity history was merged into the current game before this qualification.
This slice proves that the normal `Rat.Expedition.sln` Authoring package can load,
edit, save and reopen the native library and that native source updates work on
owned copies. It does not claim that the normal game loads or renders these assets;
runtime ownership, scale migration and the playable Dremma location remain slices 2
and 3.

## Current-editor result

`scripts/stride/verify-dremma-library.ps1` launched two invocation-owned instances
of the pinned Release Game Studio against `games/rat-expedition/Rat.Expedition.sln`.
The runner used an invocation-specific MCP server publish and the official Python
MCP SDK. The opt-in qualification assembly added no public tool. Each process
advertised the accepted 19-tool surface, where import/reimport remain explicitly
unsupported MCP operations.

The first run at `build/mcp/dremma-library-20260910-133221-451/` proved the native
operations, but independent review found that its C# saved-state snapshot used the
default JSON representation of `Vector3` and `Color4`. That representation retained
only `IsNormalized` for positions and `{}` for colors, so the fresh-reopen comparison
did not cover the values it claimed. The corrected replacement evidence is
`build/mcp/dremma-library-20260910-134227-247/`.

| Evidence | Observed result |
|---|---|
| `result.json`, `result.json.client.json` | PASS; PID 40332; 73 MCP calls; native catalogue/edit/place/reimport/Undo/Redo/Save |
| `reopen.json`, `reopen.json.client.json` | PASS; fresh PID 9896; 30 MCP calls; clean saved assets, IDs, references, instances, values and source hashes matched; changed-position negative oracle passed |
| `library-integrity.json`, `library-before.json`, `library-after.json` | PASS; all 170 original library files remained byte-identical: 97 native assets plus 73 source/evidence files |
| `saved-fixture-0/`, `saved-fixture-1/` | Six saved native fixture assets and two changed source copies archived outside Authoring after fresh reopen; no fixture directory remains in `Assets/` or `Resources/` |

The live editor loaded all 97 CanalCity assets: 12 models, 12 prefabs, 45 textures,
11 materials, 2 door animations, 12 skeletons, one scene, one compositor and one
procedural preview model. The bounded MCP resource catalogue exposed 81 of them;
skeletons, animations, compositor and procedural model are outside its resource
allowlist. The hook also required the Blender source, Noto OFL credit, representative
model FBX and both door animation FBX sources. It counted all 73 Resource files.

The MCP client opened the owned qualification scene through the editor API. It
changed the shared material color from `(0.8, 0.45, 0.12, 1)` to
`(0.2, 0.35, 0.8, 1)`, verified native Undo and Redo, placed two instances of one
prefab at offsets `(-3, 0, 0)` and `(3, 0, 0)`, then verified distinct entity and
instance IDs, native base-part links, placement Undo/Redo and session Save. Both
instances retained the same prefab → model → shared-material chain.

The corrected snapshot serializes explicit `X/Y/Z` and `R/G/B/A` scalars. The MCP
client now asserts exact root positions `(-3, 0, 0)` and `(3, 0, 0)` before Save,
one remaining `(-3, 0, 0)` root after placement Undo, both roots after Redo, and
both roots again after the fresh process reopen. The reopen hook also clones the
saved snapshot, changes one placed root's X value by `+1`, and requires the same
deep comparison used for acceptance to reject it. `reopen.json` records
`negativeSnapshotPositionOracle: true`; both saved snapshots contain the exact
positions and material color scalars.

## Real source updates on owned copies

The opt-in hook copied existing library files to
`Resources/DremmaLibraryQualification/`; it never changed a file below
`Resources/CanalCity/` or `Assets/CanalCity/`. After the first native Save it
replaced those copies with other valid, existing CanalCity sources and called the
real `AssetSourcesViewModel.UpdateAssetFromSource` path inside one named native Undo
transaction.

- The FBX copy changed from `lantern_amber.fbx` SHA-256
  `12b034335950a5333df8dabe9d25b7dbac8c7f72e4121652bc5b100165028349`
  to `bridge_arch.fbx` SHA-256
  `0b0b7eca1fabd57c2ccc40cb3498684cc077c950838cd9c5dc9ba8ff23223059`.
  The real importer changed model metadata from three material slots to the seven
  bridge slots (`CC_stone`, `CC_stone_dark`, `CC_stone_light`, `CC_burgundy`,
  `CC_brass`, `CC_iron`, `CC_amber`). The model ID, prefab reference and the three
  shared material references matched by slot name remained stable.
- The PNG copy changed from `amber_basecolor.png` SHA-256
  `3e3214303d819a44c13b36565dac837d9d06c2e5d177673ef673554a3b9973e1`
  to the real 1600×1200 `foundation_modules.png` SHA-256
  `eaf747c79c79c35204e4b3b8057843312db87b7e02bef42951da23419bd7be5a`.
  Native source tracking accepted a different source hash while the texture asset
  ID and its material dependency remained stable. This establishes source/dependency
  handling; it is not a viewport image or runtime texture-rendering claim.

The MCP client then undid the combined reimport, observed the original three FBX
slots, redid it, observed all seven replacement slots, saved, and disconnected.
The second Game Studio process compared canonical package-relative source paths,
native IDs, accepted source-hash values, material slots/references, shared color,
scene part/base identities and both prefab instances with the saved snapshot.

## Repository and dependency boundary

The normal solution already contains `Rat.Expedition.Authoring`; no preview-only
project is needed to browse the library. The isolated CanalCity preview remains
supplementary evidence and is not treated as normal-game acceptance. No runtime,
scale, camera, traversal or game-content behavior changed in this slice.

The engine checkout remained clean at
`c0b9065d6e902b45d4d3a5c656318c70df53a6f3`. Locked Stride package versions and
content hashes in both Authoring lock files were unchanged. The worktree's isolated
cache was seeded only after `restore-authoring-cohort.ps1` verified all 43 required
archives against those committed hashes; no global cache clearing, package repack
or content-hash update occurred.

## Parent acceptance

Independent read-only re-review approved `abfb271cdf6092f29963d69d92cb68be4eb1816b`:
the sole P2 snapshot gap is fixed; no remaining findings. The accepted evidence is
`build/mcp/dremma-library-20260910-134227-247/` (73+30 calls,19 tools).
Parent canonical full Release command `powershell -NoProfile -ExecutionPolicy Bypass
-File scripts/stride/build.ps1` completed exit0 on clean unchanged engine pin
`c0b9065d6e902b45d4d3a5c656318c70df53a6f3`, SDK10.0.300,61.41s,5 warnings,0 errors.
Evidence: `C:/5_gamedev/stride/logs/rat-foundation/20260910-134658-422/result.json`
and adjacent build.log/build.binlog. Editor:
`C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
Vault/projection checks pass. Accepted library history is published to main under
the standing instruction; native game resources/scale/location remain next slices.

## Verification limits

These runs prove actual Game Studio/API behavior without screenshots or simulated
input. They do not prove manual GUI gestures, viewport appearance, compiled game
resource ownership, executable rendering, gameplay scale, Dremma traversal, a
clean machine or remote CI. The full Release editor build and independent review
are complete as recorded above.
