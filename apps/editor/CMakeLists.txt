add_executable(rat-editor
  main.cpp
  editor_app.cpp
  editor_app.hpp
  imgui_bgfx.cpp
  imgui_bgfx.hpp
)

set(_rat_imgui_shader_out "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
file(MAKE_DIRECTORY "${_rat_imgui_shader_out}")
file(COPY
  "${RAT_BGFX_IMGUI_SHADER_DIR}/vs_ocornut_imgui.bin.h"
  "${RAT_BGFX_IMGUI_SHADER_DIR}/fs_ocornut_imgui.bin.h"
  DESTINATION "${_rat_imgui_shader_out}"
)

target_include_directories(rat-editor
  PRIVATE
    "${_rat_imgui_shader_out}"
)

target_link_libraries(rat-editor
  PRIVATE
    rat::engine
    imgui
    glfw
    bgfx
    bx
)

target_compile_definitions(rat-editor
  PRIVATE
    GLFW_INCLUDE_NONE
)

if(WIN32)
  target_link_libraries(rat-editor PRIVATE gdi32 user32 shell32)
endif()

set_target_properties(rat-editor PROPERTIES
  OUTPUT_NAME "rat-editor"
)
