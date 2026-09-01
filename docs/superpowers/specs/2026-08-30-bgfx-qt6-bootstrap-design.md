# Design: rat-engine bootstrap (bgfx + Qt 6)

Date: 2026-08-30  
Status: **Superseded** by `2026-08-31-glfw-imgui-shell-design.md` (Qt shell removed)

## Goal

Ship a minimal Windows editor shell that hosts a bgfx viewport inside Qt 6 Widgets, with engine code isolated from Qt.

## Decisions

| Topic | Choice | Rationale |
|-------|--------|-----------|
| Shell | Mini-editor (menu + docks + viewport) | Matches product direction; viewport path reusable later |
| Dependencies | CMake FetchContent via `bgfx.cmake` (bx/bimg/bgfx); Qt 6 external | Less infra than vcpkg; Qt is large and usually preinstalled |
| Qt ↔ bgfx | Native `QWidget` + `winId()` → `platformData.nwh` | Avoids QOpenGLWidget context fights with D3D11 |
| Renderer (MVP) | Direct3D11 on Windows | Default bgfx path on Win64 |
| Language | C++20 | Qt 6 + modern CMake |

## Architecture

- `rat_engine` — no Qt includes in public headers. Owns `Renderer` (bgfx lifecycle) and `Engine` (frame orchestration).
- `rat-editor` — Qt application: `MainWindow`, placeholder docks, `BgfxViewport` that supplies native window handle and drives `Engine::frame()`.

### Frame loop

1. Qt event loop owns the thread.
2. `QTimer` (~60 Hz) or paint-driven updates call into `Engine`.
3. Before `bgfx::init`, call `bgfx::renderFrame()` once so bgfx runs single-threaded on the Qt thread.
4. Resize → `bgfx::reset(width, height, flags)`.

### Non-goals (MVP)

Assets, custom shaders beyond clear/dbgText, ECS, input map, Linux/macOS, QML.

## Acceptance

- CMake configures with Qt 6 on PATH / `CMAKE_PREFIX_PATH`.
- `rat-editor` shows colored clear + `bgfx` debug text `rat-engine`.
- Resize keeps rendering alive.
- Engine sources compile without Qt.
