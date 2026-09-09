# Design: GLFW + Dear ImGui editor shell

Date: 2026-08-31  
Status: Accepted  
Supersedes: `2026-08-30-bgfx-qt6-bootstrap-design.md`

## Goal

Replace Qt in `rat-editor` with GLFW (window/input) and Dear ImGui docking (panels), keeping `rat_engine` Qt-free and presenting via bgfx into the GLFW HWND.

## Decisions

| Topic | Choice |
|-------|--------|
| Window | GLFW 3, `GLFW_CLIENT_API = GLFW_NO_API` |
| UI | Dear ImGui `docking` branch |
| ImGui render | Custom `imgui_bgfx` using bgfx embedded shaders (`vs/fs_ocornut_imgui` from bgfx examples) |
| Input | Official `imgui_impl_glfw` (input only) |
| Deps | FetchContent (glfw, imgui) alongside existing bgfx.cmake |

## Frame loop

1. `glfwPollEvents` + ImGui GLFW new frame  
2. ImGui NewFrame → dockspace (Hierarchy / Inspector stubs)  
3. `Engine::begin_frame` (clear view 0 + dbgText)  
4. `imgui_bgfx::render` (view 255)  
5. `Engine::end_frame` → `bgfx::frame`

## Acceptance

- Build without Qt.  
- Visible ImGui docks + `rat-engine` dbgText.  
- Resize updates bgfx.  
- No `#include <Q*>`.
