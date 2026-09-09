# rat-engine

Rat Expedition is now developed in Stride/C#. Start with the
[current game and Windows package workflow](games/rat-expedition/README.md),
[pinned Stride source workflow](docs/stride-source-workflow.md), and
[game design](vault/game/GDD.md). Open [vault/](vault/) in Obsidian for decisions
and the work queue. P1.1/P1.1a are complete; the remaining P1 traversal slices
are tracked in the vault.

This repository also preserves the C++ engine/editor below, which remains buildable
and verified, and the [historical C++ game prototype](apps/game/README.md).
The current game lives in `games/rat-expedition/`; Stride's pinned upstream source
is a separate checkout and is not vendored here.

C++20 game engine and in-process map editor using **bgfx**, **GLFW**, and **Dear ImGui**.
The editor supports Play/Edit, JSON maps, event graphs, undo/redo, voxel terrain,
rotating ramps, slabs and ladder traversal. Headless tests use the same simulation
entrypoint as the editor.

## Build and verify

Requirements: CMake 3.24+, Ninja, a C++20 compiler, Python 3 for repository checks,
and network access for the first FetchContent configure.

On Windows install Visual Studio C++ Build Tools, then from any PowerShell prompt:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify.ps1
```

The wrapper discovers Visual Studio through `vswhere` and imports its x64 environment.
Pass `-Python <python-executable>` to override Python detection, and `-Preset dev-debug`
for Debug. `-ConfigureOnly` validates docs/scripts and configures without building.
Release outputs are under `build/dev-release`.

Linux needs GCC/Clang with C++20 plus X11/OpenGL development headers. On Ubuntu
install `ninja-build pkg-config xorg-dev libgl1-mesa-dev libglu1-mesa-dev`.
In a compiler-ready shell on either platform:

```sh
cmake --preset dev-release
cmake --build --preset dev-release
ctest --preset dev-release
python3 scripts/check_vault.py
python3 -m unittest discover -s scripts/tests
```

On Windows use `python` or its explicit path in place of `python3`.
Launch `build/dev-release/apps/editor/rat-editor.exe` on Windows, or
`build/dev-release/apps/editor/rat-editor` on Linux. Existing non-preset `build/`
trees are still supported and are not overwritten by presets.

## Headless, launch paths and packages

`cmake --preset headless-release`, `cmake --build --preset headless-release`, and
`ctest --preset headless-release` build/test core and editor logic without fetching
bgfx, GLFW, ImGui or miniaudio. On Windows the same workflow is
`powershell -ExecutionPolicy Bypass -File scripts/verify.ps1 -Preset headless-release`.
The configure check rejects a build tree contaminated by prior graphics dependencies.
`RAT_BUILD_RENDERER=ON` with editor/tests off builds the renderer without window/UI/audio
libraries. Editor and the GUI-runner option require the renderer.

The ordinary editor accepts:

```sh
rat-editor --data-root /path/to/data --user-data-dir /path/to/profile --map /path/to/map.json
```

All three switches are optional. Relative paths resolve against the launch directory
once. Data defaults to `data/` beside the executable; the initial map is
`data/maps/grey_yard.json`. Writable saves, logs, snapshots and ImGui settings use
Windows LocalAppData/rat-engine or Linux XDG_DATA_HOME/rat-engine
(HOME/.local/share/rat-engine fallback). `RAT_LOG_PATH`, if set, overrides only the
log path and also resolves once. Paths and Windows process arguments are UTF-8.
The build copies bundled data beside the editor, so map authoring uses that copy
unless an explicit `--map` points to another project.

Build Release, then install/package:

```sh
cmake --install build/dev-release --prefix build/package-stage
cpack --config build/dev-release/CPackConfig.cmake -B build/packages
```

The ZIP contains the executable, adjacent data, licence notices and the required
MSVC runtime libraries on Windows. It needs no repository-relative resource paths.
`rat_editor_app` is shared with `rat-editor-gui-tests`. The `gui-release` preset
builds the full 40-scenario suite; Windows uses verified WARP, Linux uses Xvfb/Mesa.
See [GUI automation](docs/gui-automation.md) for installed-package acceptance.
See [launch contract](docs/editor-launch.md) for the app integration interface.

## Acceptance and diagnostics

On Windows run `powershell -ExecutionPolicy Bypass -File scripts/verify.ps1 -Preset gui-release`
for the editor, ordinary tests and full GUI suite. `dev-release` explicitly keeps GUI off.
On Linux run the `gui-release` configure/build presets, then
`LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe xvfb-run -a ctest --preset gui-release`.
The Linux-only `headless-sanitizers` preset enables Debug ASan/UBSan on our targets.

`-Benchmarks` on the Windows wrapper (or `-DRAT_BUILD_BENCHMARKS=ON` at configure)
adds the headless baseline executable. See [benchmark methodology and results](docs/benchmark.md)
and [CI/static-analysis contract](docs/continuous-integration.md). Hosted Linux,
sanitizer and source-absent package results require an observed workflow run;
local Windows checks alone do not establish those outcomes.

## Compiled boundaries

| Target | Responsibility / dependencies |
| --- | --- |
| `rat_core` | SimulationSession, event VM, map authoring/compilation, replay, asset/entity scaffolding; nlohmann/json |
| `rat_engine` | Rendering; core + bgfx/bx/bimg |
| `rat_editor_logic` | Headless editor document and frame coordination |
| `rat_editor_app` | Shared real editor app/panels, GLFW, ImGui, renderer and audio composition |
| `rat-editor` | Process arguments and the ordinary app entrypoint |
| `rat-editor-gui-tests` | Real app/input/panels automation, captures and read-only observations |
| `rat-benchmark` | Optional headless tick/bake/apply/history-memory baseline |
| `rat_tests` | Catch2 unit, mechanics, regression and grey_yard smoke scenarios |

CMake enforces the core's platform/graphics link isolation; see
[library graph](cmake/library-graph.md). Proposed separate runtime, authoring and
asset libraries remain future work; see [architecture roadmap](docs/architecture-roadmap.md).

## Current limits

The scoped replay, save and editor-authoring defects from the
[initial audit](docs/audits/2026-09-07-project-review.md) have been fixed and
independently reviewed under the [stabilization plan](docs/superpowers/plans/2026-09-07-stabilization.md).
Local Windows acceptance passed the full GUI suite and installed-package suite;
see the [verification evidence](docs/audits/2026-09-07-stabilization-evidence.md).
EventTouch remains unsupported and cross-map transfer has no map loader; both
are rejected at compile/Save/Apply while authored drafts remain repairable.
Asset IDs and an in-memory loader exist; a production mesh/texture importer is
still planned.

CI is configured for Windows/Linux Release, headless tests, GUI automation,
Linux Debug sanitizers, static analysis and source-absent package acceptance.
Remote CI, Linux and sanitizer execution have not been observed in this session;
local Windows results do not establish those outcomes.

## Project knowledge

Open `vault/` as an Obsidian vault for product decisions, tasks, bugs, sprints and ADRs.
Engineering schemas/specifications live in `docs/`, maps in `data/maps`, source in
`src/engine` and `apps/editor`. Follow [AGENTS.md](AGENTS.md) for work/review rules.
Project build, vault and reproduction skills live in `.agents/skills/`.
