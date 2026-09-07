# CI and static analysis

`.github/workflows/ci.yml` defines 30-minute jobs. Windows 2022 and Ubuntu 24.04
Release jobs build the editor and real GUI runner, run ordinary tests (120-second
timeouts), then the complete GUI suite (300 seconds). Windows verifies WARP;
Linux requests Mesa llvmpipe with `LIBGL_ALWAYS_SOFTWARE=1` under Xvfb. A PulseAudio
null sink supplies a deterministic device on hosted Linux. Initialization,
missing resources and scenario failures fail the job; there are no skip paths.

Fresh headless jobs on both platforms restore no build cache and install no
graphics/audio packages. Configure rejects graphics/audio targets or populated
FetchContent directories. They run the same core/editor-logic tests and publish
Release benchmark reports. Linux Debug `headless-sanitizers` instruments our
targets with ASan/UBSan, frame pointers and nonrecovering errors; CTest enables
leak detection and fatal findings. Vendor libraries are outside this target
instrumentation scope. Repository checks run vault validation and Python tests.

Release packages include editor, GUI runner, scripts/manifest, data and licences.
Separate downstream jobs download only the archive, with **no checkout**. They
assert the workspace has no source or Git tree, extract the package under a
temporary directory, and invoke its installed full suite from unrelated cwd,
without `--data-root`. The suite checks resource loading and that package files
and unrelated cwd remain unchanged. All GUI/static/test reports upload on failure.

Static analysis pins the Python-distributed `clang-tidy==22.1.8` tool. Run:

```text
python scripts/run_clang_tidy.py --build-dir build/headless-release --artifact-dir build/tidy
```

The runner selects exact `CMakeFiles/rat_core.dir` and `rat_editor_logic.dir`
compile-database entries, rejects an empty/missing target selection and deduplicates
files. It runs `--checks=-*,clang-analyzer-* --warnings-as-errors=* --quiet`, three
workers by default, 120 seconds per file, with per-file logs and a JSON report.
This is the Clang analyzer check family, not a claim that all style/readability
checks are enabled. There are no diagnostic suppressions or `--fix` operations.
`--executable` overrides the binary; `--command-json` supplies a JSON argv array
for wrappers such as `["uv","tool","run","--from","clang-tidy==22.1.8","clang-tidy"]`.
Use a compiler-ready developer shell for an MSVC compile database. When a shell
rewrites embedded quotes, pass the array via Python `subprocess.run` arguments.

Local acceptance artifacts live under `build/stabilization/`. Workflow syntax can
be checked with [actionlint](https://github.com/rhysd/actionlint). Committed workflow
configuration and local Windows results do not mean remote CI, Linux, sanitizers
or a fresh no-checkout runner have executed. Those results remain explicitly
unverified until an actual hosted run is observed.
