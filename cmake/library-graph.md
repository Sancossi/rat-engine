# Library graph

Gameplay stays free of windowing and GPU backends. CMake enforces this on `rat_core` via `RatCoreLinkCheck.cmake` (configure + `rat_core_link_check` + ctest `rat_core_no_platform_graphics`).

```
nlohmann_json
      ^
  rat_core              gameplay / maps / Clock / FileStore — json only
     ^    ^             no GLFW, ImGui, bgfx, Win32 (user32/gdi32)
rat_engine  rat_editor_logic
     ^    ^             engine: bgfx + bx + bimg
      rat_editor_app    logic: EditorDocument + FrameCoordinator + launch resolution
          ^             app: glfw + imgui + miniaudio (+ Win32 platform libs)
       rat-editor       ordinary process entrypoint
```

| Target | Links |
| --- | --- |
| `rat_core` | `nlohmann_json` |
| `rat_engine` | `rat_core` + bgfx + bx + bimg |
| `rat_editor_logic` | `rat_core` |
| `rat_editor_app` | `rat_editor_logic` + `rat_engine` + glfw + imgui + miniaudio |
| `rat-editor` | `rat_editor_app` |

`rat_tests` links `rat_core` + `rat_editor_logic` + Catch2 (headless, no GLFW/ImGui).

`RAT_BUILD_RENDERER` gates rat_engine/bgfx. Editor or GUI enables app/GLFW/ImGui/audio.
The headless preset disables all three; its configure check verifies no graphics/audio
targets or FetchContent directories exist. Editor logic and tests remain independent.
