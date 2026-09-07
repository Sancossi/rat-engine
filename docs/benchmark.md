# Performance baseline

Build `rat-benchmark` with `-DRAT_BUILD_BENCHMARKS=ON`. It links only core and
editor logic; the headless preset needs no graphics or audio dependencies.
Run `python scripts/run_benchmark.py --executable BUILD/benchmarks/rat-benchmark
--map data/maps/grey_yard.json --output report.json` (append `.exe` on Windows).

The report includes compiler/build mode, source commit and tracked-change flag,
OS/architecture/processor/logical CPU count and capture time. Measurements use
`steady_clock`, microseconds, nearest-rank percentiles `ceil(p*N)-1`. This is a
descriptive baseline, with no arbitrary timing gate. Compare Release runs on the
same machine and power configuration; scheduling, antivirus and other processes
affect tails. No claim about universal production budgets follows from one run.

Fixtures are the shipped grey_yard and deterministic 16/32/64 square grids with
8/32/128 parallel wait events and blockers, plus sparse raised occupancy. The
report records actual counts. Inputs alternate X movement every 120 ticks and
jump every 240; dt is 1/120 seconds. Each session executes 1,000 warmup ticks and
10,000 measured ticks. Timing excludes input construction and sample insertion.
Terrain and collision bake, hot apply to fresh targets, and fresh session load
are measured independently for 100 iterations after ten warmup iterations;
construction of empty target objects is outside those timings. Bake timings
include destruction of the resulting temporary geometry. Results are consumed
in the output so the compiler cannot discard the work.

Memory phases perform 100 changed terrain edits, add one event and make 100
graph replacements, undo 50, redo 25, then execute/commit a ten-command stroke.
These measure retained command history, not peak scratch memory or process RSS.
`estimated_retained_bytes` includes concrete object `sizeof`, vector capacities
(including unused slots), live elements' nested owned allocations and string
capacity plus terminator when outside the string's inline storage. It visits
legacy branches/routes, graph nodes/edges/conditions and all MapData vectors.
History includes undo/redo/stroke commands, composite children and both stroke
snapshot buffers. Document map, optional preview, clean snapshot and error
buffers are reported separately; their inline fields are counted exactly once.

Allocator overhead, opaque `std::function` target allocations, temporary work
allocations and unowned services are excluded. It is an estimate of known
retained bytes, not an allocator census. JSON snapshot strings already owned by
the document/history count as real buffers; freshly serializing a map is never
used as a substitute for measuring its memory. `clear()` can retain container
and snapshot capacity; instrumentation deliberately preserves this behavior.
