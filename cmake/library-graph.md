# Library graph

Gameplay stays free of windowing and GPU backends. CMake enforces this on `rat_core` via `RatCoreLinkCheck.cmake` (configure + `rat_core_link_check` + ctest `rat_core_no_platform_graphics`).

```
nlohmann_json
      ^
  rat_core          gameplay / maps / input math — json only
      ^             no GLFW, ImGui, bgfx, Win32 (user32/gdi32)
  rat_engine        present: bgfx + bx + bimg
      ^
  rat-editor        apps/editor: glfw + imgui (+ Win32 user32/gdi32/shell32)
```

| Target | Links |
| --- | --- |
| `rat_core` | `nlohmann_json` |
| `rat_engine` | `rat_core` + bgfx + bx + bimg |
| `rat-editor` | `rat_engine` + glfw + imgui |

`rat_tests` links `rat_core` + Catch2 only (headless).
