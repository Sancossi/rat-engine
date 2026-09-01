# rat-engine

Game engine bootstrap: **bgfx** renderer + **GLFW** window + **Dear ImGui** mini-editor.

CI (GitHub Actions) builds `rat_core`, `rat-editor`, and Catch2 on **Windows** and **Linux**. Tests are headless; the editor GUI is not launched.

## Layout

| Path | Role |
|------|------|
| `src/engine` | `rat_core` (logic) + `rat_engine` (bgfx present) |
| `apps/editor` | `rat-editor` — GLFW shell, ImGui docks, bgfx present |
| `data/maps` | JSON maps / events (schema in `docs/schemas/`) |
| `docs/sprint1-acceptance.md` | Sprint 1 playthrough checklist |
| `tests` | Catch2 unit + headless mechanics tests |
| `cmake/Dependencies.cmake` | FetchContent: bgfx.cmake, GLFW, ImGui, Catch2, nlohmann/json |
| `cmake/library-graph.md` | Library graph: `rat_core` → json; `rat_engine` → core + bgfx; editor → engine + glfw + imgui |
| `docs/superpowers/specs/` | Design notes |
| `vault/` | Obsidian hub: wiki, GDD, tasks, bugs, ADR (open this folder as a vault) |

## Library graph

| Target | Links |
|--------|--------|
| `rat_core` | `nlohmann_json` only (no GLFW, ImGui, bgfx, Win32) |
| `rat_engine` | `rat_core` + bgfx + bx + bimg |
| `rat-editor` | `rat_engine` + glfw + imgui (`apps/editor`) |

CMake writes the `rat_core` link closure and fails configure / `rat_core_link_check` / ctest `rat_core_no_platform_graphics` if a forbidden library appears. See `cmake/library-graph.md`.

## Prerequisites

**Windows**

1. Visual Studio 2022/2025 Build Tools with C++ workload
2. CMake 3.24+ and Ninja
3. Network on first configure (FetchContent)

No Qt install required.

**Linux**

1. g++ (C++20), CMake 3.24+, Ninja
2. X11 / OpenGL headers so GLFW and bgfx can **compile** (Ubuntu: `ninja-build pkg-config xorg-dev libgl1-mesa-dev libglu1-mesa-dev`)
3. Network on first configure (FetchContent)

`rat-editor` on Linux uses GLFW **X11** native window/display handles (`nwh` / `ndt`). CI installs those headers on `ubuntu-latest` and does not run the editor (no display / no xvfb). A Wayland-only environment is not required.

## Build

Same configure line on Windows and Linux. On Windows, use an **x64 Native Tools** / VS developer prompt:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Engine-only:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRAT_BUILD_EDITOR=OFF
cmake --build build --target rat_engine
```

Run (Windows):

```powershell
.\build\apps\editor\rat-editor.exe
```

Run (Linux):

```bash
./build/apps/editor/rat-editor
```

You should see ImGui Hierarchy/Inspector docks and a dark-blue clear with `rat-engine` debug text.

## Tests

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRAT_BUILD_TESTS=ON
cmake --build build --target rat_tests
ctest --test-dir build --output-on-failure
```

Headless grey_yard smoke (no GLFW/bgfx):

```powershell
.\build\tests\rat_tests.exe "[smoke]"
```

```bash
./build/tests/rat_tests "[smoke]"
```

- **Unit:** `GameState` and future pure logic (no GLFW/bgfx).
- **Mechanics:** headless event/quest scenarios (stub ready; fill with Event runtime v1).
- **Smoke:** `[smoke]` loads `data/maps/grey_yard.json` without a window.
- Disable with `-DRAT_BUILD_TESTS=OFF`.

## Knowledge vault (Obsidian)

Product wiki, GDD, sprints, tasks, bugs, and ADRs live in **`vault/`** (git). Engineering schemas and implementation specs stay in `docs/`.

In Obsidian: **File → Open folder as vault** and choose the `vault` directory (not the repo root). Optional community plugin: Dataview (listed in `vault/.obsidian/community-plugins.json`; install from Obsidian if you want table views). The agent reads/writes markdown + YAML and does not need the app.

Do not treat Notion as the source of truth for this project.

## Notes

- GLFW window uses `GLFW_NO_API`. Windows passes HWND to bgfx (D3D11); Linux passes X11 `Display*` / `Window` (`ndt` / `nwh`).
- ImGui is rendered through a small `imgui_bgfx` bridge (bgfx embedded shaders).
- `rat_core` has no GLFW/ImGui/bgfx; `rat_editor_logic` is also headless (`EditorDocument` / `FrameCoordinator`); `rat_engine` adds rendering. Graph: `cmake/library-graph.md`. CMake fails configure/ctest if `rat_core` grows a GLFW/ImGui/bgfx/Win32 link (`rat_core_link_check`, ctest `rat_core_no_platform_graphics`).
- GitHub Actions (`.github/workflows/ci.yml`) builds and runs Catch2 on `windows-latest` and `ubuntu-latest`.
