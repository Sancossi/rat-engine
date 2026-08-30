# rat-engine

Game engine bootstrap: **bgfx** renderer + **GLFW** window + **Dear ImGui** mini-editor (Windows x64 / Direct3D11).

## Layout

| Path | Role |
|------|------|
| `src/engine` | `rat_engine` library (no UI framework in public API) |
| `apps/editor` | `rat-editor` — GLFW shell, ImGui docks, bgfx present |
| `cmake/Dependencies.cmake` | FetchContent: bgfx.cmake, GLFW, ImGui |
| `docs/superpowers/specs/` | Design notes |

## Prerequisites (Windows)

1. Visual Studio 2022/2025 Build Tools with C++ workload  
2. CMake 3.24+  
3. Network on first configure (FetchContent)

No Qt install required.

## Build

From a **x64 Native Tools** / VS developer prompt:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Engine-only:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRAT_BUILD_EDITOR=OFF
cmake --build build --target rat_engine
```

Run:

```powershell
.\build\apps\editor\rat-editor.exe
```

You should see ImGui Hierarchy/Inspector docks and a dark-blue clear with `rat-engine` debug text.

## Notes

- GLFW window uses `GLFW_NO_API`; HWND is passed to bgfx (D3D11).  
- ImGui is rendered through a small `imgui_bgfx` bridge (bgfx embedded shaders).  
- `rat_engine` stays free of GLFW/ImGui includes in public headers.
