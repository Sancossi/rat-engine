# Runtime capabilities and replay schema 2

Sprint 15 stage 4, implemented 2026-09-07.

## Runtime acceptance and graph authority

Compilation, Apply and Save reject Event touch at the page's `/trigger` and
cross-map Transfer Player at its `/map_id` (legacy commands) or
`/graph/nodes/N/params/map_id` (graph nodes). Same-map transfer retains surface
height sampling. Raw loading/serialization permits these authored drafts so the
editor can repair them. Event touch remains an enum value for compatibility; its
UI option is disabled and existing pages display the unsupported diagnostic.

A present graph is authoritative, including an explicitly empty graph. Only an
absent graph migrates from legacy commands. Stale commands neither execute nor
veto runtime capability validation for an authoritative graph. Empty graphs are
valid no-ops under the existing graph contract; malformed edges still fail graph
validation. The RuntimeMap overload validates before replacing the current map;
rejection preserves it and emits a warning. Transfer execution also guards against
unsupported targets and nonfinite positions before changing GameState.

## Recording and playback API

`begin_recording(session, seed)` returns `ReplayRecordResult {ok, code, error,
recording}` and requires a fresh session. `record_input_sequence` constructs that
session from the map, complete SimulationConfig and starting PlayerBody. The
low-level `record_tick` builder checks the current header, input and sequential
tick ID (including the previous tail ID) before appending in amortized constant
time. It does not rescan earlier ticks; full read/write/play validation rejects
historical data that a caller subsequently modified. Callers supplying their own
checksums own that data.

Freshness compares canonical **state bytes**, not hash equality, with a session
created by constructor → load same map → set same starting player. This detects
saved progress retained at tick zero, pending input, altered jump/VM state and
off-map self-switches. A history that leaves exactly the same authoritative state
is equivalent to a fresh session. No mid-save or mid-VM resume is advertised.

Read/write return explicit status and error categories (`Io`, `Format`,
`Incompatible`, `InvalidSession`). There is no optional-empty success or checksum
sentinel. All header/input/tick/session validation runs before executing a tick.
Playback returns `ok` for a valid completed playback, separately reports
`checksums_match`, first divergence, actual final checksum and `ticks_executed`.
A recorded checksum of zero is compared like any other value. An empty recording
is valid, but still checks the map fingerprint and fresh-session contract.

## Schema and compatibility

Schema 2 requires exactly `schema_version`, `header`, `ticks`. Header requires:

- `map_id`, `map_fingerprint`, `runtime_version`, `checksum_version`, `seed`;
- complete `config`: dt, app mode, all eleven JumpTuning fields, interaction-buffer
  duration and maximum step height;
- complete `start_player`: x/y/z, half extent and speed.

Every tick requires its ID, checksum and complete InputFrame (both movement pairs
and all button edges). IDs are contiguous from one. Duplicate JSON keys, missing
or unknown fields, wrong numeric/boolean types, narrowing, nonfinite values and
unsupported versions fail. Old replay requires re-recording. Integers retain full
uint64 precision; zero fingerprints are not wildcard values.

The accepted dt range is `(0, 0.2]` seconds; substep duration is positive with at
most 4096 substeps per tick. Tuning/durations/speed are nonnegative, player extent
positive, jump cut in `[0,1]`, movement axes within `[-1,1]` with rounding tolerance.
Replay creates its session with the recorded configuration; playback into an
existing session rejects a different configuration. Seed currently salts the
checksum; the simulation has no random generator to seed.

## Authoritative state and canonical encoding

Read-only value snapshots cover:

- Complete player, JumpState, SimulationConfig and tick ID;
- Game map/position, every switch/variable, all self-switch entries including
  off-map events, ordered inventory;
- Pending jump/interaction flags and interaction-buffer seconds;
- Foreground and ordered parallel interpreters: event/page/node, wait frames,
  route index, paid route budget, message wait, parallel/autorun/finished flags;
- Active message, sorted touch-inside/parallel-started/autorun-lock sets;
- Sorted NPC overlays with tile, facing and world x/z, and previous-player touch
  sampling state.

Pointers, warning logs, profiling times, collision caches and graphics are excluded.
The per-update dt scratch value is overwritten before use; the authoritative dt is
the simulation configuration. Derived parallel/command counters are not VM state.

Encoding uses explicit type tags, big-endian 64-bit scalars and collection/string
lengths, lexical object keys and preserved array order. Floats encode IEEE-754
double values of the stored float fields; signed zero normalizes to zero.
FNV-1a over canonical bytes and a fixed-width seed produces the checksum. It is a
determinism diagnostic, not a cryptographic integrity mechanism.

The map fingerprint uses the same encoding over compiled runtime geometry and
graph parameters/routes, preserving semantic event/page order. It normalizes
legacy schema heights, removes stale commands, node canvas layout, asset debug
names and source revision. Node IDs/ordering and graph edges remain part of the
contract. Changing geometry or an effect invalidates the recording; moving nodes
on the canvas does not.

## Validation evidence

Regression scenarios cover original grey_yard replay, nondefault dt/full config
roundtrip, uint64 seed, malformed headers/ticks with zero executed steps, checksum
zero divergence, changed geometry/graph with an empty recording, layout-only and
source-revision compatibility, tick-zero saved progress rejection, off-map switches,
pending input/buffer differences, unordered insertion invariance, NPC motion,
route-index differences at equal waits, and previous-player touch state. Runtime
tests cover repairable unsupported drafts, precise paths, forged RuntimeMap safety
and empty graphs never reviving legacy effects. Interactive GUI verification belongs
to stage 6; compiler/headless checks alone do not establish the disabled UI behavior.

## Incremental append measurement (2026-09-07)

Windows x64 Release preset, MSVC, AMD Ryzen 9 9950X3D (32 logical processors).
Command: `build/dev-release/tests/rat_tests.exe "Streaming append scaling observations" --success`.
The timed section calls the real `record_tick` without pre-reserving its vector;
fresh-session setup and the final full validation are outside that section.

| Appends | Seconds | Microseconds per append |
| --- | --- | --- |
| 1,000 | 0.011954 | 11.954 |
| 4,000 | 0.046952 | 11.738 |
| 16,000 | 0.182801 | 11.425 |

These local observations support the linear total-work implementation; they are
not a timing gate. The benchmark has no timing assertion. A separate regression
compares streaming and sequence-helper output byte-for-byte and checks that full
persistence/playback validation still rejects historical input modified by a caller.
Full verification after this fix: 689/689 C++ cases, Python tests and vault checks.
