# Native asset actions — 2026-09-09

Bounded A1.3 implementation: `asset_rename`, `asset_delete`, `prefab_place`.
[Card](../../vault/production/tasks/feat-stride-game-studio-authoring.md),
[contract](../stride-native-resource-library-spec.md),
[MCP usage](../../tools/stride-mcp/README.md).
The engine remains pinned to `c0b9065d6e902b45d4d3a5c656318c70df53a6f3`;
this slice needs no engine patch or game package/content change.
Independent review and parent full Release acceptance are complete for this slice.

## Behavior

- Rename accepts one safe filename segment on the existing resource allowlist,
  rejects Windows/native name collisions before mutation, and calls native
  `AssetViewModel.Name` under an owned operation lease and Undo transaction.
  Native reference analysis updates model/material and prefab base URLs.
- Delete rejects incoming native Reference dependencies, including Archetype and
  prefab base parts, reporting dependent IDs. It preflights `CanDelete`, then uses
  `SessionViewModel.DeleteItems([asset], skipConfirmation: true)` with the same
  owned transaction discipline. It never forces reference clearing. Undo/Redo is
  session-wide; native Save performs disk deletion.
- Placement requires an explicit editable Scene/Prefab pair in the same package.
  It rejects dependency cycles, absent/nonlocal references, malformed hierarchies,
  excessive size (256 parts/16 roots) and nonfinite combined root positions before
  graph insertion. Native `CreatePrefabInstance(prefab.Url)` preserves base links
  and remaps entity/component IDs. `AddPartToAsset` registers native hierarchy;
  Quantum then overrides each root position with original position + requested
  offset. One native transaction controls the insertion and offsets.
- Diagnostics advertise 19 tools and these three bounded actions. Import/reimport
  remain unsupported MCP operations. C# source and GameSettings are outside the
  resource allowlist.

## Original failure and corrected fixture

Original draft run `build/mcp/asset-actions-20260909-152225-451/` stopped on
the first valid placement. Test-only FirstChanceException logging reproduced the
exact failure in `build/mcp/asset-actions-20260909-153226-971/result.json.exceptions.log`:

```text
System.NullReferenceException
EntityHierarchyAssetBase.GetParent(Entity) : 26
AssetCompositeHierarchyPropertyGraph.FindBestInsertIndex(...) : 494
AssetCompositeHierarchyPropertyGraph.PartAddedInBaseAsset(...) : 689
AssetCompositeHierarchyPropertyGraph.NotifyPartAdded(...) : 763
AssetCompositeHierarchyPropertyGraph.RootPartsChanged(...) : 779
ObjectNode.Add(...) : 132
AssetCompositeHierarchyPropertyGraph.AddPartToAsset(...) : 160
EditorBridge.PlacePrefab / OwnedAction
```

The cycle-negative fixture manually assigned a BasePart to an unrelated new root.
Its Transform component ID did not match native inheritance metadata; native
reconciliation removed that unmatched Transform. Adding a root to Target then
propagated into this invalid dependent fixture, where `GetParent` dereferenced
the missing Transform. The adapter's legitimate placement was not the cause.

The fixture now calls `target.CreateDerivedAsset(target.Url)` and transfers the
derived native hierarchy to the dependent prefab. It explicitly checks inherited
Transforms after registration. Target retains that dependent fixture during both
placements, Undo/Redo, reference updates and save/reopen; all entities, transforms,
components, parent links, native base-part IDs and instance IDs are inspected
again. No exception is swallowed and no null-check engine workaround was added.
The opt-in diagnostic only exists in the qualification assembly.

## Executed verification

All commands below ran locally on Windows. They use real Game Studio sessions,
the official MCP SDK and invocation-specific stdio executables, without captures
or simulated input. The runners stop their own Process records and archive their
temporary fixtures outside production assets. Unknown pre-existing stdio servers
were not stopped or overwritten.

| Command / evidence | Observed result |
|---|---|
| `verify-asset-actions.ps1 -McpPython C:/5_gamedev/rat-engine-mcp/build/mcp-parent-client/Scripts/python.exe`; `build/mcp/asset-actions-20260909-153923-506/` | PASS, exit 0; editing and fresh-process reopen; 19-tool discovery in each session; 160 + 12 calls, 19 + 1 expected errors, zero captures |
| Earlier complete action qualification `build/mcp/asset-actions-20260909-153621-788/` | PASS, exit 0; 160 + 12 calls; native Save and Close each rejected during 7 owned asset actions |
| `verify-resource-api.ps1 -McpPython C:/5_gamedev/rat-engine-mcp/build/mcp-parent-client/Scripts/python.exe`; `build/mcp/resource-api-20260909-153748-586/result.json` | PASS, exit 0; 101 calls, 13 expected errors, 19 tools, zero captures; native batch success/mixed, source-update Save/Close/Undo/Redo/disk and Destroy regressions pass |
| `verify-session-state.ps1`; `build/mcp/session-state-20260909-153844-182/result.json` | PASS, exit 0; native Save/Close, concurrent queued edits, dirty/disk/history and Destroy regression |
| `dotnet run --project tools/stride-mcp/Rat.StrideMcp.Tests -c Release -p:RestoreLockedMode=true` | PASS, exit 0; path/junction checks, dispatcher cancellation and framing/input bounds |
| `python scripts/check_vault.py`; `python -m unittest discover -s scripts/tests` | PASS, exit 0; vault valid and 25 Python tests |

Action evidence is in `result.json`, `result.json.client.json`, `reopen.json`,
`reopen.json.client.json`, `saved-snapshot.json` and `saved-fixture/` under the run.
Expected rejections cover unsafe/reserved/colliding names, source C#, unknown and
stale IDs, referenced deletion (including Archetype and placed prefab), wrong
asset types, dependency cycle, incomplete vector and finite-float addition overflow.
Rejected actions preserve target inspection, dirty state, revision and Undo/Redo
targets; the stale-revision check additionally confirms revision remains unchanged.

Two instances start from source root `(1,2,3)` and use offsets `(2,0,0)` and
`(-2,0,0)`, producing `(3,2,3)` and `(-1,2,3)`. Both inherit a shared model update;
that model references the edited shared material. A local model/material reference
override on the first instance survives a subsequent base update independently.
Renaming the model, material and prefab updates references; deleting unreferenced
material can be undone/redone, and saving its deletion survives fresh reopen.

Snapshots validate each native URL against its inspected target in that session,
then compare semantic data by native IDs. This deliberately accepts package-prefix
changes across native reload and irrelevant collection ordering. Source hierarchy,
instance hierarchy, model/material properties and independent overrides are all
included in the saved comparison.

## Parent acceptance

Independent read-only review approved implementation `15a75cc7af7f2722d559890225ac4433af247366`
against `975f653`, with no actionable blockers. The reviewer inspected native
rename validation, deletion modal branches and dependency preflights, owned abort
and poison guards, native prefab identity/inheritance, qualification and evidence.

Parent canonical `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build.ps1`
completed with exit 0 on the unchanged clean engine pin. Evidence:
`C:/5_gamedev/stride/logs/rat-foundation/20260909-154241-614/result.json`,
`build.log` and `build.binlog`; SDK 10.0.300, 73.28 seconds, 952 warnings, 0 errors.
Editor: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
The warning count describes this actual build, not new warnings introduced by the adapter.
Parent vault/projection checks and 25 Python tests passed. The two unrelated vault
files retain their original hashes and are excluded from these commits.

Bugs found: the invalid native inheritance test fixture described above is fixed;
no open findings from independent review. This accepted milestone is published to
project main under the standing instruction. Stride main/pin remains unchanged.

## Verification limits

This proves API behavior in actual editor processes, not manual GUI gestures,
viewport appearance, game runtime resource rendering, sound, import/reimport or
the full A1.3 library. No remote CI or clean-machine claim is made. The game's
Release ZIP was not rebuilt for this adapter-only slice. The full Release editor
was rebuilt by the parent as recorded above. Runtime game
code, dependency locks and pinned package hashes remain unchanged.
