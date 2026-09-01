# Library graph

Gameplay stays free of windowing and GPU backends. CMake enforces this on `rat_core` via `RatCoreLinkCheck.cmake` (configure + `rat_core_link_check` + ctest `rat_core_no_platform_graphics`).

```
nlohmann_json
      ^
  rat_core              gameplay / maps / input math — json only
     ^    ^             no GLFW, ImGui, bgfx, Win32 (user32/gdi32)
rat_engine  rat_editor_logic
     ^    ^             engine: bgfx + bx + bimg
      rat-editor        logic: EditorDocument + FrameCoordinator
                        editor: glfw + imgui (+ Win32 user32/gdi32/shell32)
```

| Target | Links |
| --- | --- |
| `rat_core` | `nlohmann_json` |
| `rat_engine` | `rat_core` + bgfx + bx + bimg |
| `rat_editor_logic` | `rat_core` |
| `rat-editor` | `rat_editor_logic` + `rat_engine` + glfw + imgui |

`rat_tests` links `rat_core` + `rat_editor_logic` + Catch2 (headless, no GLFW/ImGui).
