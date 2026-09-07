# Atomic storage and save schema 2

Accepted implementation contract: Sprint 15 stage 2 (2026-09-07).

## File transaction

`FileStore::write_atomic(path, bytes)` is an explicit required adapter operation.
Map and game saves use it; ordinary logs/debug writes retain `write`. No implicit
fallback to truncating `write` is allowed. `AtomicFileOps` exposes the transaction
steps for failure injection; `atomic_write` is the common production/test algorithm.

The OS adapter exclusively creates a unique temporary file beside the target,
handles partial writes, and checks flush and close. If the target exists, its
bytes are copied to another checked temporary file and atomically replace `.bak`.
Failure to create that backup aborts the main save. A platform replacement then
publishes the completed main temporary file without deleting the old target first.
All owned temporary files are cleaned up on failure. First save creates no backup;
subsequent saves retain one previous version, without `.bak.bak` files. After a
failed final replacement, `.bak` may already equal the unchanged main file.

Windows uses private `CreateFileW` / `WriteFile` / `FlushFileBuffers` / `CloseHandle`
and `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` calls. POSIX uses exclusive `open`,
checked `write` / `fsync` / `close`, and same-directory `rename`. Public core
headers expose no platform handles. Paths are UTF-8. Writers to the same document
must be serialized by the owner; this API is not a concurrent-writer arbitration
protocol. It does not promise persistence through hardware failure or a power cut
(directory durability and the two-file transaction are not claimed).

## Save format and compatibility

New files are JSON with exactly these required fields:

```json
{
  "schema_version": 2,
  "map_id": "grey_yard",
  "position": {"x": 1, "y": 0, "z": 2},
  "switches": [{"id": 1, "value": true}],
  "variables": [{"id": 7, "value": 42}],
  "self_switches": [{"event_id": "npc", "bits": 8}],
  "inventory": [{"id": "door key", "quantity": 1, "key_item": true}]
}
```

Object keys and collection IDs must be unique. Unknown/missing fields, unknown
versions, wrong JSON types, integral narrowing/truncation and nonfinite/out-of-range
float positions fail. Map and string IDs are nonempty; Unicode and spaces are
preserved. Switch/variable IDs span uint32; variable values span int; self-switch
bits span 0–15. Inventory quantities are nonnegative (zero entries remain meaningful
in the existing inventory model); order and key-item flags are preserved. Unordered
state collections are emitted in stable ID order.

Valid legacy `RATSAVE1` remains readable. It requires exactly one nonempty `map`
and one finite `pos`; optional records are complete with no trailing tokens,
duplicate IDs, invalid flags or overflowing values. Legacy token-based item and
self-switch IDs keep their original no-space restriction. JSON removes that
limitation for new saves. Loading builds a temporary state and only commits after
complete validation. Public `apply_loaded_game` rejects empty/mismatched map IDs
and nonfinite coordinates before touching runtime/player state.

Save semantics remain progress and player position. They do not resume a VM,
movement route, jump or input buffer in the middle of execution. The former test
that accepted an empty-map save now asserts rejection; the isolated route test
sets its map ID before saving, as SimulationSession normally does.

## Map safety and authoring boundary

Map schemas 1–5 remain supported. A shared checked JSON number reader validates
before narrowing. Each grid has a limit of 16,777,216 cells (64 MiB of float heights);
dimensions and integer extents are checked before height allocation, including
schema-1 fallback in programmatic compilation. Grid membership uses widened
subtraction. `validate_map_structure` checks numeric safety, IDs and occupancy
bounds without compiling graph drafts. Raw loading and serialization use this
boundary; `validate_map_document` / compilation retain full semantic graph checks.
File save compiles before serializing and publishing. This avoids both recursive
serialization/compilation and treating an incomplete graph gesture as invalid I/O.

## Validation

Regression tests inject failures into the common transaction's create, partial
write, finish, backup and replacement steps, checking old main bytes, backup
semantics, cleanup and absence of subsequent operations. Real OS tests verify
successive replacements and a backup destination failure. Save tests cover legacy
corruption, JSON required fields/types/IDs/versions, Unicode/order/float precision,
and transactional rejection. Map tests cover NaN across geometry and transfers,
integer/float narrowing, oversized grids, draft graph separation, and exact
`/occupancy/0` diagnostics. Windows full Release build and 656 Catch2 cases passed;
Linux and interactive GUI are not verified by this stage.
