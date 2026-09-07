# rat-engine

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

## Compiled boundaries

| Target | Responsibility / dependencies |
| --- | --- |
| `rat_core` | SimulationSession, event VM, map authoring/compilation, replay, asset/entity scaffolding; nlohmann/json |
| `rat_engine` | Rendering; core + bgfx/bx/bimg |
| `rat_editor_logic` | Headless editor document and frame coordination |
| `rat-editor` | GLFW shell, ImGui panels, rendering/audio composition |
| `rat_tests` | Catch2 unit, mechanics, regression and grey_yard smoke scenarios |

CMake enforces the core's platform/graphics link isolation; see
[library graph](cmake/library-graph.md). Proposed separate runtime, authoring and
asset libraries remain future work; see [architecture roadmap](docs/architecture-roadmap.md).

## Current limits

Sprint 15 implements the [approved stabilization plan](docs/superpowers/plans/2026-09-07-stabilization.md).
Existing replay, save and editor authoring paths have recorded correctness gaps;
the [audit](docs/audits/2026-09-07-project-review.md) describes initial findings.
EventTouch is not executable and cross-map transfer has no map loader. Asset IDs
and an in-memory loader exist; a production mesh/texture importer is still planned.

CI currently builds Windows/Linux and runs headless tests. GUI input automation,
sanitizers, isolated headless configuration and relocatable packaging are scheduled
in stabilization; headless tests do not verify the desktop interface. Turning
`RAT_BUILD_EDITOR=OFF` currently still builds/fetches renderer dependencies.
Data/user-write path portability is also part of that work.

## Project knowledge

Open `vault/` as an Obsidian vault for product decisions, tasks, bugs, sprints and ADRs.
Engineering schemas/specifications live in `docs/`, maps in `data/maps`, source in
`src/engine` and `apps/editor`. Follow [AGENTS.md](AGENTS.md) for work/review rules.
Project build, vault and reproduction skills live in `.agents/skills/`.
