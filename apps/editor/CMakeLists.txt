add_library(rat_editor_logic
  editor_document.cpp
  editor_document.hpp
  frame_coordinator.cpp
  frame_coordinator.hpp
)

add_library(rat::editor_logic ALIAS rat_editor_logic)

target_include_directories(rat_editor_logic
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(rat_editor_logic
  PUBLIC
    rat::core
)

target_compile_features(rat_editor_logic PUBLIC cxx_std_20)

if(WIN32)
  target_compile_definitions(rat_editor_logic PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(NOT RAT_BUILD_EDITOR)
  return()
endif()

add_executable(rat-editor
  main.cpp
  editor_app.cpp
  editor_app.hpp
  imgui_bgfx.cpp
  imgui_bgfx.hpp
  platform/glfw_host.cpp
  platform/glfw_host.hpp
  panels/blocker_panel.cpp
  panels/blocker_panel.hpp
  panels/terrain_panel.cpp
  panels/terrain_panel.hpp
  panels/event_panel.cpp
  panels/event_panel.hpp
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
    rat::editor_logic
    rat::engine
    imgui
    glfw
    bgfx
    bx
)

target_compile_definitions(rat-editor
  PRIVATE
    GLFW_INCLUDE_NONE
    RAT_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

if(WIN32)
  target_link_libraries(rat-editor PRIVATE gdi32 user32 shell32)
endif()

set_target_properties(rat-editor PROPERTIES
  OUTPUT_NAME "rat-editor"
)
